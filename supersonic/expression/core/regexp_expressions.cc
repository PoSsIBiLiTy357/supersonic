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

#include "supersonic/expression/core/regexp_expressions.h"

#include "supersonic/utils/std_namespace.h"
#include "supersonic/utils/macros.h"
#include "supersonic/utils/stringprintf.h"
#include "supersonic/base/exception/exception.h"
#include "supersonic/base/exception/exception_macros.h"
#include "supersonic/base/exception/result.h"
#include "supersonic/base/infrastructure/types.h"
#include "supersonic/expression/base/expression.h"
#include "supersonic/expression/core/regexp_bound_expressions.h"
#include "supersonic/expression/core/regexp_bound_expressions_internal.h"
#include "supersonic/expression/core/regexp_evaluators.h"  // IWYU pragma: keep
#include "supersonic/expression/infrastructure/basic_expressions.h"
#include "supersonic/expression/infrastructure/expression_utils.h"
#include "supersonic/expression/proto/operators.pb.h"
#include "supersonic/expression/vector/expression_traits.h"
#include "supersonic/proto/supersonic.pb.h"
#include "supersonic/utils/strings/stringpiece.h"

namespace supersonic {

class BufferAllocator;
class TupleSchema;

namespace {

// The RegExp expressions. They differ from the standard abstract setting in
// having a state - the regexp pattern. This is not an argument (that is - its
// not an expression, as we do not want to compile the pattern each time), but
// really a state.
// TODO(onufry): This could be done through expressions traits, by making them
// stateful and passing an instance of the traits through the building of an
// expression. But that looks like a giant piece of work, and probably isn't
// worth it unless we hit a large number of stateful expressions.
template<OperatorId operation_type>
class RegexpExpression : public UnaryExpression {
 public:
  RegexpExpression(unique_ptr<const Expression> arg,
                   const StringPiece& pattern)
      : UnaryExpression(std::move(arg)),
        pattern_(pattern.data(), pattern.length()) {}

  virtual string ToString(bool verbose) const {
    return UnaryExpressionTraits<operation_type>::FormatDescription(
        child_expression_->ToString(verbose));
  }

 private:
  virtual FailureOrOwned<BoundExpression> CreateBoundUnaryExpression(
      const TupleSchema& schema,
      BufferAllocator* const allocator,
      rowcount_t row_capacity,
      unique_ptr<BoundExpression> child) const {
    DataType child_type = GetExpressionType(child.get());
    if (child_type != STRING) {
      THROW(new Exception(ERROR_ATTRIBUTE_TYPE_MISMATCH,
          StringPrintf("Invalid argument type (%s) to %s, STRING expected",
                       GetTypeInfo(child_type).name().c_str(),
                       ToString(true).c_str())));
    }
    return BoundGeneralRegexp<operation_type>(
        std::move(child), pattern_, allocator, row_capacity);
  }

  string pattern_;

  DISALLOW_COPY_AND_ASSIGN(RegexpExpression);
};

class RegexpExtractExpression : public UnaryExpression {
 public:
  RegexpExtractExpression(unique_ptr<const Expression> arg,
                          const StringPiece& pattern)
      : UnaryExpression(std::move(arg)),
        pattern_(pattern.data(), pattern.length()) {}

  virtual string ToString(bool verbose) const {
    return StringPrintf("REGEXP_EXTRACT(%s)",
                        child_expression_->ToString(verbose).c_str());
  }

 private:
  virtual FailureOrOwned<BoundExpression> CreateBoundUnaryExpression(
      const TupleSchema& schema,
      BufferAllocator* const allocator,
      rowcount_t row_capacity,
      unique_ptr<BoundExpression> child) const {
    DataType child_type = GetExpressionType(child.get());
    if (child_type != STRING) {
      THROW(new Exception(ERROR_ATTRIBUTE_TYPE_MISMATCH,
          StringPrintf("Invalid argument type (%s) to %s, STRING expected",
                       GetTypeInfo(child_type).name().c_str(),
                       ToString(true).c_str())));
    }
    return BoundRegexpExtract(std::move(child), pattern_, allocator,
                              row_capacity);
  }

  string pattern_;

  DISALLOW_COPY_AND_ASSIGN(RegexpExtractExpression);
};

class RegexpReplaceExpression : public BinaryExpression {
 public:
  RegexpReplaceExpression(unique_ptr<const Expression> haystack,
                          const StringPiece& needle,
                          unique_ptr<const Expression> substitute)
      : BinaryExpression(std::move(haystack), std::move(substitute)),
        pattern_(needle.data(), needle.length()) {}

  virtual string ToString(bool verbose) const {
    return BinaryExpressionTraits<OPERATOR_REGEXP_REPLACE>::FormatDescription(
        left_->ToString(verbose), right_->ToString(verbose));
  }

 private:
  virtual FailureOrOwned<BoundExpression> CreateBoundBinaryExpression(
      const TupleSchema& schema,
      BufferAllocator* const allocator,
      rowcount_t row_capacity,
      unique_ptr<BoundExpression> left,
      unique_ptr<BoundExpression> right) const {
    DataType left_type = GetExpressionType(left.get());
    if (left_type != STRING) {
      THROW(new Exception(ERROR_ATTRIBUTE_TYPE_MISMATCH,
          StringPrintf("Invalid argument type (%s) as first argument to %s, "
                       "STRING expected",
                       GetTypeInfo(left_type).name().c_str(),
                       ToString(true).c_str())));
    }
    DataType right_type = GetExpressionType(right.get());
    if (right_type != STRING) {
      THROW(new Exception(ERROR_ATTRIBUTE_TYPE_MISMATCH,
          StringPrintf("Invalid argument type (%s) as last argument to %s, "
                       "STRING expected",
                       GetTypeInfo(right_type).name().c_str(),
                       ToString(true).c_str())));
    }
    return BoundRegexpReplace(std::move(left), pattern_, std::move(right),
                              allocator, row_capacity);
  }

  string pattern_;

  DISALLOW_COPY_AND_ASSIGN(RegexpReplaceExpression);
};

}  // namespace

unique_ptr<const Expression> RegexpPartialMatch(unique_ptr<const Expression> str,
                                     const StringPiece& pattern) {
  return make_unique<RegexpExpression<OPERATOR_REGEXP_PARTIAL>>(std::move(str), pattern);
}

unique_ptr<const Expression> RegexpFullMatch(unique_ptr<const Expression> str,
                                  const StringPiece& pattern) {
  return make_unique<RegexpExpression<OPERATOR_REGEXP_FULL>>(std::move(str), pattern);
}

unique_ptr<const Expression> RegexpExtract(unique_ptr<const Expression> str,
                                const StringPiece& pattern) {
  return make_unique<RegexpExtractExpression>(std::move(str), pattern);
}

unique_ptr<const Expression> RegexpReplace(unique_ptr<const Expression> haystack,
                                const StringPiece& needle,
                                unique_ptr<const Expression> substitute) {
  return make_unique<RegexpReplaceExpression>(std::move(haystack), needle,
                                              std::move(substitute));
}

}  // namespace supersonic
