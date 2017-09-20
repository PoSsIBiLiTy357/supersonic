// Copyright 2010 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: onufry@google.com (Onufry Wojtaszczyk)

#include "supersonic/expression/core/regexp_bound_expressions.h"

#include <glog/logging.h>
#include "supersonic/utils/std_namespace.h"
#include "supersonic/utils/logging-inl.h"
#include "supersonic/utils/macros.h"
#include "supersonic/utils/exception/failureor.h"
#include "supersonic/base/exception/exception.h"
#include "supersonic/base/exception/exception_macros.h"
#include "supersonic/base/exception/result.h"
#include "supersonic/base/infrastructure/bit_pointers.h"
#include "supersonic/base/infrastructure/block.h"
#include "supersonic/base/infrastructure/tuple_schema.h"
#include "supersonic/expression/base/expression.h"
#include "supersonic/expression/core/regexp_evaluators.h"  // IWYU pragma: keep
#include "supersonic/expression/infrastructure/basic_bound_expression.h"
#include "supersonic/expression/infrastructure/expression_utils.h"
#include "supersonic/expression/proto/operators.pb.h"
#include "supersonic/expression/vector/binary_column_computers.h"
#include "supersonic/expression/vector/expression_traits.h"
#include "supersonic/proto/supersonic.pb.h"
#include "supersonic/utils/strings/strcat.h"
#include <re2/re2.h>

namespace supersonic {

class BufferAllocator;

namespace {

template<OperatorId op>
class BoundRegexpExpression : public BoundUnaryExpression {
 public:
  // Takes over the ownership of the pattern.
  BoundRegexpExpression(const string& output_name,
                        BufferAllocator* const allocator,
                        unique_ptr<BoundExpression> arg,
                        unique_ptr<const RE2> pattern)
      : BoundUnaryExpression{
            CreateSchema(output_name, BOOL, arg.get(),
                         UnaryExpressionTraits<op>::can_return_null
                             ? NULLABLE
                             : NOT_NULLABLE),
            allocator, STRING, std::move(arg)},
        pattern_(std::move(pattern)) {}

 private:
  virtual EvaluationResult DoEvaluate(const View& input,
                                      const BoolView& skip_vectors) {
    CHECK_EQ(1, skip_vectors.column_count());
    my_block()->ResetArenas();
    bool_ptr skip_vector = skip_vectors.column(0);
    EvaluationResult result = argument()->DoEvaluate(input, skip_vectors);
    PROPAGATE_ON_FAILURE(result);

    bool* destination =
        my_block()->mutable_column(0)->template mutable_typed_data<BOOL>();

    const StringPiece* source = result.get().column(0).typed_data<STRING>();
    typename UnaryExpressionTraits<op>::basic_operator operation;

    bool selective_evaluate = SelectivityIsGreaterThan(
        skip_vector, input.row_count(),
        UnaryExpressionTraits<op>::selectivity_threshold);
    if (selective_evaluate) {
      for (int i = 0; i < input.row_count(); ++i) {
        if (!*skip_vector) {
          destination[i] = operation(*pattern_, source[i]);
        }
        ++skip_vector;
      }
    } else {
      for (int i = 0; i < input.row_count(); ++i) {
        destination[i] = operation(*pattern_, source[i]);
      }
    }
    my_view()->set_row_count(input.row_count());
    my_view()->mutable_column(0)->ResetIsNull(skip_vectors.column(0));
    return Success(*my_view());
  }

  unique_ptr<const RE2> pattern_;

  DISALLOW_COPY_AND_ASSIGN(BoundRegexpExpression);
};

class BoundRegexpExtractExpression : public BoundUnaryExpression {
 public:
  // Takes over the ownership of the pattern.
  BoundRegexpExtractExpression(const string& output_name,
                               BufferAllocator* const allocator,
                               unique_ptr<BoundExpression> arg,
                               unique_ptr<const RE2> pattern)
      : BoundUnaryExpression{CreateSchema(output_name, STRING, NULLABLE),
                             allocator, STRING, std::move(arg)},
        pattern_(std::move(pattern)) {}

 private:
  virtual EvaluationResult DoEvaluate(const View& input,
                                      const BoolView& skip_vectors) {
    my_block()->ResetArenas();
    CHECK_EQ(1, skip_vectors.column_count());
    bool_ptr skip_vector = skip_vectors.column(0);
    EvaluationResult result = argument()->DoEvaluate(input, skip_vectors);
    PROPAGATE_ON_FAILURE(result);

    StringPiece* destination =
        my_block()->mutable_column(0)->mutable_typed_data<STRING>();

    const StringPiece* source = result.get().column(0).typed_data<STRING>();

    for (int i = 0; i < input.row_count(); ++i) {
      if (!*skip_vector) {
        re2::StringPiece re2_source(source[i].data(), source[i].length());
        re2::StringPiece re2_destination;
        *skip_vector |=
            !RE2::PartialMatch(re2_source,
                               *pattern_,
                               &re2_destination);
        destination[i].set(re2_destination.data(), re2_destination.length());
      }
      ++skip_vector;
    }

    my_view()->set_row_count(input.row_count());
    my_view()->mutable_column(0)->ResetIsNull(skip_vectors.column(0));
    return Success(*my_view());
  }

  unique_ptr<const RE2> pattern_;

  DISALLOW_COPY_AND_ASSIGN(BoundRegexpExtractExpression);
};

class BoundRegexpReplaceExpression : public BoundBinaryExpression {
 public:
  // Takes over the ownership of the pattern.
  BoundRegexpReplaceExpression(const string& output_name,
                               BufferAllocator* const allocator,
                               unique_ptr<BoundExpression> left,
                               unique_ptr<BoundExpression> right,
                               unique_ptr<const RE2> pattern)
      : BoundBinaryExpression{CreateSchema(output_name, STRING, left.get(), right.get()),
                              allocator, std::move(left), STRING, std::move(right), STRING},
        pattern_(std::move(pattern)) {}

 private:
  virtual EvaluationResult DoEvaluate(const View& input,
                                      const BoolView& skip_vectors) {
    my_block()->ResetArenas();
    CHECK_EQ(1, skip_vectors.column_count());
    bool_ptr skip_vector = skip_vectors.column(0);
    EvaluationResult left_result = left()->DoEvaluate(input, skip_vectors);
    PROPAGATE_ON_FAILURE(left_result);
    EvaluationResult right_result = right()->DoEvaluate(input, skip_vectors);
    PROPAGATE_ON_FAILURE(right_result);

    StringPiece* destination =
        my_block()->mutable_column(0)->mutable_typed_data<STRING>();

    binary_column_computers::CheckAndNull<OPERATOR_REGEXP_REPLACE, STRING,
        STRING, STRING> nuller;
    FailureOrVoid nulling_result = nuller(left_result.get().column(0),
                                          right_result.get().column(0),
                                          skip_vector,
                                          input.row_count());
    PROPAGATE_ON_FAILURE(nulling_result);

    const StringPiece* haystack =
        left_result.get().column(0).typed_data<STRING>();
    const StringPiece* substitute =
        right_result.get().column(0).typed_data<STRING>();
    string temp;
    operators::RegexpReplace replace_operator;

    if (!SelectivityIsGreaterThan(
        skip_vector, input.row_count(),
        BinaryExpressionTraits<
            OPERATOR_REGEXP_REPLACE>::selectivity_threshold)) {
      for (int i = 0; i < input.row_count(); ++i) {
        destination[i] = replace_operator(
            haystack[i], *pattern_, substitute[i], temp,
            my_block()->mutable_column(0)->arena());
      }
    } else {
      for (int i = 0; i < input.row_count(); ++i) {
        if (!*skip_vector) {
          destination[i] = replace_operator(
              haystack[i], *pattern_, substitute[i], temp,
              my_block()->mutable_column(0)->arena());
        }
        ++skip_vector;
      }
    }
    my_view()->set_row_count(input.row_count());
    my_view()->mutable_column(0)->ResetIsNull(skip_vectors.column(0));
    return Success(*my_view());
  }

  unique_ptr<const RE2> pattern_;

  DISALLOW_COPY_AND_ASSIGN(BoundRegexpReplaceExpression);
};

}  // namespace

// ------------------------ Internal -------------------------------------------

template<OperatorId operation_type>
FailureOrOwned<BoundExpression> BoundGeneralRegexp(unique_ptr<BoundExpression> child,
                                                   const StringPiece& pattern,
                                                   BufferAllocator* allocator,
                                                   rowcount_t max_row_count) {
  string name = UnaryExpressionTraits<operation_type>::FormatDescription(
      child->result_schema().attribute(0).name());

  DataType input_type = GetExpressionType(child.get());
  if (input_type != STRING) {
    THROW(new Exception(
        ERROR_ATTRIBUTE_TYPE_MISMATCH,
        StrCat("Invalid input type (", GetTypeInfo(input_type).name(),
               "), STRING expected in ", name)));
  }
  unique_ptr<const RE2> pattern_(new RE2(pattern.ToString()));
  if (!pattern_->ok()) {
    string message = StrCat("Malformed regexp: ", pattern, ", parse error: ",
                            pattern_->error());
    THROW(new Exception(ERROR_INVALID_ARGUMENT_VALUE, message));
  }
  return InitBasicExpression(
      max_row_count,
      make_unique<BoundRegexpExpression<operation_type>>(
          name, allocator, std::move(child), std::move(pattern_)),
      allocator);
}

template FailureOrOwned<BoundExpression>
BoundGeneralRegexp<OPERATOR_REGEXP_PARTIAL>(unique_ptr<BoundExpression> child_ptr,
                                            const StringPiece& pattern,
                                            BufferAllocator* allocator,
                                            rowcount_t max_row_count);

template FailureOrOwned<BoundExpression>
BoundGeneralRegexp<OPERATOR_REGEXP_FULL>(unique_ptr<BoundExpression> child_ptr,
                                         const StringPiece& pattern,
                                         BufferAllocator* allocator,
                                         rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundRegexpPartialMatch(
    unique_ptr<BoundExpression> str,
    const StringPiece& pattern,
    BufferAllocator* allocator,
    rowcount_t max_row_count) {
  return BoundGeneralRegexp<OPERATOR_REGEXP_PARTIAL>(std::move(str), pattern,
                                                     allocator, max_row_count);
}

FailureOrOwned<BoundExpression> BoundRegexpFullMatch(
    unique_ptr<BoundExpression> str,
    const StringPiece& pattern,
    BufferAllocator* allocator,
    rowcount_t max_row_count) {
  return BoundGeneralRegexp<OPERATOR_REGEXP_FULL>(std::move(str), pattern, allocator,
                                                  max_row_count);
}

FailureOrOwned<BoundExpression> BoundRegexpExtract(
    unique_ptr<BoundExpression> child,
    const StringPiece& pattern,
    BufferAllocator* allocator,
    rowcount_t max_row_count) {
  string name =
      UnaryExpressionTraits<OPERATOR_REGEXP_EXTRACT>::FormatDescription(
          GetExpressionName(child.get()));

  FailureOrVoid input_check = CheckExpressionType(STRING, child.get());
  PROPAGATE_ON_FAILURE(input_check);
  auto pattern_ = make_unique<RE2>(pattern.ToString());
  if (!pattern_->ok()) {
    string message = StrCat("Malformed regexp: ", pattern, ", parse error: ",
                            pattern_->error(), " in ", name);
    THROW(new Exception(ERROR_INVALID_ARGUMENT_VALUE, message));
  }
  return InitBasicExpression(
      max_row_count,
      make_unique<BoundRegexpExtractExpression>(
          name, allocator, std::move(child), std::move(pattern_)),
      allocator);
}

FailureOrOwned<BoundExpression> BoundRegexpReplace(
    unique_ptr<BoundExpression> haystack,
    const StringPiece& pattern,
    unique_ptr<BoundExpression> substitute,
    BufferAllocator* allocator,
    rowcount_t max_row_count) {
  string name =
      BinaryExpressionTraits<OPERATOR_REGEXP_REPLACE>::FormatDescription(
          GetExpressionName(haystack.get()), GetExpressionName(substitute.get()));

  FailureOrVoid haystack_check = CheckExpressionType(STRING, haystack.get());
  PROPAGATE_ON_FAILURE(haystack_check);
  FailureOrVoid sub_check = CheckExpressionType(STRING, substitute.get());
  PROPAGATE_ON_FAILURE(sub_check);
  auto pattern_ = make_unique<RE2>(pattern.ToString());
  if (!pattern_->ok()) {
    string message = StrCat("Malformed regexp: ", pattern, ", parse error: ",
                            pattern_->error(), " in ", name);
    THROW(new Exception(ERROR_INVALID_ARGUMENT_VALUE, message));
  }
  return InitBasicExpression(
      max_row_count,
      make_unique<BoundRegexpReplaceExpression>(
          name, allocator, std::move(haystack), std::move(substitute),
          std::move(pattern_)),
      allocator);
}

}  // namespace supersonic
