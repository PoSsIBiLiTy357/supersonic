// Copyright 2010 Google Inc.  All Rights Reserved
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

#include "supersonic/expression/core/elementary_expressions.h"

#include "supersonic/utils/std_namespace.h"
#include "supersonic/utils/macros.h"
#include "supersonic/utils/stringprintf.h"
#include "supersonic/utils/exception/failureor.h"
#include "supersonic/base/exception/exception.h"
#include "supersonic/base/exception/exception_macros.h"
#include "supersonic/base/exception/result.h"
#include "supersonic/base/infrastructure/types.h"
#include "supersonic/expression/base/expression.h"
#include "supersonic/expression/core/elementary_bound_expressions.h"
#include "supersonic/expression/infrastructure/basic_expressions.h"
#include "supersonic/expression/infrastructure/expression_utils.h"
#include "supersonic/expression/proto/operators.pb.h"
#include "supersonic/expression/templated/abstract_expressions.h"
#include "supersonic/expression/templated/bound_expression_factory.h"
#include "supersonic/expression/templated/cast_expression.h"
#include "supersonic/expression/vector/expression_traits.h"

namespace supersonic {

class BufferAllocator;
class TupleSchema;

namespace {

// ----------------------------------------------------------------------------
// Unary expressions.
//

// A separate class for expressions that only transform the input from one type
// into another. They are bound in a different than usual way - if the
// from_type and to_type are equal, we simply pass the source expression along.
// This, along with the need to store the to_type somewhere, warrants a
// separate class for the Cast Expression (note that there is no separate
// class for the BoundChangeTypeExpression).
template<OperatorId op>
class ChangeTypeExpression : public UnaryExpression {
 public:
  ChangeTypeExpression(DataType type, unique_ptr<const Expression> source)
      : UnaryExpression(std::move(source)),
        to_type_(type) {}
  virtual ~ChangeTypeExpression() {}

  virtual string ToString(bool verbose) const {
    return UnaryExpressionTraits<op>::FormatDescription(
        child_expression_->ToString(verbose), to_type_);
  }

 private:
  virtual FailureOrOwned<BoundExpression> CreateBoundUnaryExpression(
      const TupleSchema& input_schema,
      BufferAllocator* const allocator,
      rowcount_t row_capacity,
      unique_ptr<BoundExpression> child) const;

  DataType to_type_;
  DISALLOW_COPY_AND_ASSIGN(ChangeTypeExpression);
};

template<>
FailureOrOwned<BoundExpression>
ChangeTypeExpression<OPERATOR_CAST_QUIET>::CreateBoundUnaryExpression(
    const TupleSchema& input_schema,
    BufferAllocator* const allocator,
    rowcount_t row_capacity,
    unique_ptr<BoundExpression> child) const {
  DataType from_type = GetExpressionType(child.get());
  if (from_type == to_type_) return Success(std::move(child));
  return CreateUnaryNumericExpression<OPERATOR_CAST_QUIET>(allocator,
      row_capacity, std::move(child), to_type_);
}

template<>
FailureOrOwned<BoundExpression>
ChangeTypeExpression<OPERATOR_PARSE_STRING_QUIET>::CreateBoundUnaryExpression(
    const TupleSchema& input_schema,
    BufferAllocator* const allocator,
    rowcount_t row_capacity,
    unique_ptr<BoundExpression> child) const {
  return BoundParseStringQuiet(to_type_, std::move(child), allocator, row_capacity);
}

template<>
FailureOrOwned<BoundExpression>
ChangeTypeExpression<OPERATOR_PARSE_STRING_NULLING>::CreateBoundUnaryExpression(
    const TupleSchema& input_schema,
    BufferAllocator* const allocator,
    rowcount_t row_capacity,
    unique_ptr<BoundExpression> child) const {
  return BoundParseStringNulling(to_type_, std::move(child), allocator, row_capacity);
}

// ----------------------------------------------------------------------------
// Multi-parameter expressions. The reason this exists is that the logic to
// check the argument number is even needs to live somewhere, and it's better to
// fail at expression creation time (as all other expressions fail) instead of
// failing at binding time.
class CaseExpression : public Expression {
 public:
  explicit CaseExpression(unique_ptr<const ExpressionList> arguments)
      : arguments_(std::move(arguments)) {}

  virtual FailureOrOwned<BoundExpression> DoBind(
      const TupleSchema& input_schema,
      BufferAllocator* allocator,
      rowcount_t max_row_count) const {
    // Bind all arguments.
    FailureOrOwned<BoundExpressionList> bound_arguments = arguments_->DoBind(
        input_schema, allocator, max_row_count);
    PROPAGATE_ON_FAILURE(bound_arguments);
    size_t num_arguments = bound_arguments->size();

    if (num_arguments < 4) {
      THROW(new Exception(
          ERROR_ATTRIBUTE_COUNT_MISMATCH,
          StringPrintf("Bind failed: "
                       "CASE needs at least 4 arguments (%zd provided).",
                       num_arguments)));
    }

    if (num_arguments % 2 != 0) {
      THROW(new Exception(
          ERROR_ATTRIBUTE_COUNT_MISMATCH,
          StringPrintf("Bind failed: "
                       "CASE needs even number of arguments (%zd provided). "
                       "Maybe you forgot the ELSE argument?",
                       num_arguments)));
    }

    return BoundCase(bound_arguments.move(), allocator, max_row_count);
  }

  virtual string ToString(bool verbose) const {
    return StringPrintf("CASE(%s)", arguments_->ToString(verbose).c_str());
  }

 private:
  unique_ptr<const ExpressionList> const arguments_;
  DISALLOW_COPY_AND_ASSIGN(CaseExpression);
};

}  // namespace

// ----------------------------------------------------------------------------
// Expressions instantiation:
unique_ptr<const Expression> CastTo(DataType to_type, unique_ptr<const Expression> child) {
  return InternalCast(to_type, std::move(child), false);
}

unique_ptr<const Expression> ParseStringQuiet(DataType to_type,
                                   unique_ptr<const Expression> child) {
  return make_unique<ChangeTypeExpression<OPERATOR_PARSE_STRING_QUIET>>(
      to_type, std::move(child));
}

unique_ptr<const Expression> ParseStringNulling(DataType to_type,
                                     unique_ptr<const Expression> child) {
  return make_unique<ChangeTypeExpression<OPERATOR_PARSE_STRING_NULLING>>(
      to_type, std::move(child));
}

unique_ptr<const Expression> Case(unique_ptr<const ExpressionList> arguments) {
  return make_unique<CaseExpression>(std::move(arguments));
}

unique_ptr<const Expression> Not(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(
      std::move(argument), &BoundNot, "(NOT $0)");
}

unique_ptr<const Expression> And(unique_ptr<const Expression> left,
                      unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundAnd, "($0 AND $1)");
}

unique_ptr<const Expression> Xor(unique_ptr<const Expression> left,
                      unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundXor, "($0 XOR $1)");
}

unique_ptr<const Expression> Or(unique_ptr<const Expression> left,
                     unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundOr, "($0 OR $1)");
}

unique_ptr<const Expression> AndNot(unique_ptr<const Expression> left,
                         unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundAndNot, "$0 !&& $1");
}

unique_ptr<const Expression> If(unique_ptr<const Expression> condition,
                     unique_ptr<const Expression> then,
                     unique_ptr<const Expression> otherwise) {
  return CreateExpressionForExistingBoundFactory(
      std::move(condition), std::move(then), std::move(otherwise), &BoundIf,
      "IF $0 THEN $1 ELSE $2");
}

unique_ptr<const Expression> NullingIf(unique_ptr<const Expression> condition,
                            unique_ptr<const Expression> then,
                            unique_ptr<const Expression> otherwise) {
  return CreateExpressionForExistingBoundFactory(
      std::move(condition), std::move(then), std::move(otherwise),
      &BoundIfNulling, "IF $0 THEN $1 ELSE $2");
}

unique_ptr<const Expression> IsNull(unique_ptr<const Expression> expression) {
  return CreateExpressionForExistingBoundFactory(
      std::move(expression), &BoundIsNull, "ISNULL($0)");
}

unique_ptr<const Expression> IfNull(unique_ptr<const Expression> expression,
                         unique_ptr<const Expression> substitute) {
  return CreateExpressionForExistingBoundFactory(
      std::move(expression), std::move(substitute), &BoundIfNull, "IFNULL($0, $1)");
}

unique_ptr<const Expression> BitwiseNot(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(
      std::move(argument), &BoundBitwiseNot, "(~$0)");
}

unique_ptr<const Expression> BitwiseAnd(unique_ptr<const Expression> a,
                             unique_ptr<const Expression> b) {
  return CreateExpressionForExistingBoundFactory(
      std::move(a), std::move(b), &BoundBitwiseAnd, "($0 & $1)");
}

unique_ptr<const Expression> BitwiseAndNot(unique_ptr<const Expression> a,
                                unique_ptr<const Expression> b) {
  return CreateExpressionForExistingBoundFactory(
      std::move(a), std::move(b), &BoundBitwiseAndNot, "(~$0 & $1)");
}

unique_ptr<const Expression> BitwiseOr(unique_ptr<const Expression> a,
                            unique_ptr<const Expression> b) {
  return CreateExpressionForExistingBoundFactory(
      std::move(a), std::move(b), &BoundBitwiseOr, "($0 | $1)");
}

unique_ptr<const Expression> BitwiseXor(unique_ptr<const Expression> a,
                             unique_ptr<const Expression> b) {
  return CreateExpressionForExistingBoundFactory(
      std::move(a), std::move(b), &BoundBitwiseXor, "($0 ^ $1)");
}

unique_ptr<const Expression> ShiftLeft(unique_ptr<const Expression> argument,
                            unique_ptr<const Expression> shift) {
  return CreateExpressionForExistingBoundFactory(
      std::move(argument), std::move(shift), &BoundShiftLeft, "($0 << $1)");
}

unique_ptr<const Expression> ShiftRight(unique_ptr<const Expression> argument,
                             unique_ptr<const Expression> shift) {
  return CreateExpressionForExistingBoundFactory(
      std::move(argument), std::move(shift), &BoundShiftRight, "($0 >> $1)");
}

}  // namespace supersonic
