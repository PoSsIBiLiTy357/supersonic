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
// Author:  onufry@google.com (Onufry Wojtaszczyk)

#include "supersonic/expression/core/comparison_expressions.h"

#include "supersonic/utils/std_namespace.h"
#include "supersonic/utils/macros.h"
#include "supersonic/utils/stringprintf.h"
#include "supersonic/base/exception/exception.h"
#include "supersonic/base/exception/exception_macros.h"
#include "supersonic/base/exception/result.h"
#include "supersonic/base/infrastructure/tuple_schema.h"
#include "supersonic/base/infrastructure/types.h"
#include "supersonic/base/infrastructure/types_infrastructure.h"
#include "supersonic/expression/base/expression.h"
#include "supersonic/expression/core/comparison_bound_expressions.h"
#include "supersonic/expression/infrastructure/basic_expressions.h"
#include "supersonic/expression/infrastructure/expression_utils.h"
#include "supersonic/expression/templated/cast_bound_expression.h"
#include "supersonic/proto/supersonic.pb.h"

namespace supersonic {

class BufferAllocator;

namespace {
// The in expression set expression, extending expression. Similar to Concat,
// this doesn't fit into the general scheme of abstract_expressions.h as it has
// an arbitrary number of arguments.
class InExpressionSetExpression : public Expression {
 public:
  InExpressionSetExpression(unique_ptr<const Expression> needle_expression,
                            unique_ptr<const ExpressionList> haystack_arguments)
      : needle_expression_(std::move(needle_expression)),
        haystack_arguments_(std::move(haystack_arguments)) {}

 private:
  virtual FailureOrOwned<BoundExpression> DoBind(
      const TupleSchema& input_schema,
      BufferAllocator* allocator,
      rowcount_t max_row_count) const {
    FailureOrOwned<BoundExpression> bound_needle =
        needle_expression_->DoBind(input_schema, allocator, max_row_count);
    PROPAGATE_ON_FAILURE(bound_needle);
    FailureOrOwned<BoundExpressionList> bound_haystack =
        haystack_arguments_->DoBind(input_schema, allocator, max_row_count);
    PROPAGATE_ON_FAILURE(bound_haystack);
    return BoundInSet(bound_needle.move(),
                      bound_haystack.move(),
                      allocator,
                      max_row_count);
  }

  virtual string ToString(bool verbose) const {
    return StrCat(needle_expression_->ToString(verbose),
                  " IN (",
                  haystack_arguments_->ToString(verbose),
                  ")");
  }

  const unique_ptr<const Expression> needle_expression_;
  const unique_ptr<const ExpressionList> haystack_arguments_;
};
}  // namespace

unique_ptr<const Expression>
In(unique_ptr<const Expression> needle_expression,
   unique_ptr<const ExpressionList> haystack_arguments) {
  return make_unique<InExpressionSetExpression>(std::move(needle_expression),
                                                std::move(haystack_arguments));
}

unique_ptr<const Expression> IsOdd(unique_ptr<const Expression> arg) {
  return CreateExpressionForExistingBoundFactory(std::move(arg), &BoundIsOdd,
                                                 "IS_ODD($0)");
}

unique_ptr<const Expression> IsEven(unique_ptr<const Expression> arg) {
  return CreateExpressionForExistingBoundFactory(std::move(arg), &BoundIsEven,
                                                 "IS_EVEN($0)");
}

unique_ptr<const Expression> NotEqual(unique_ptr<const Expression> left,
                                      unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundNotEqual, "$0 <> $1");
}

unique_ptr<const Expression> Equal(unique_ptr<const Expression> left,
                                   unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundEqual, "$0 == $1");
}

unique_ptr<const Expression> Greater(unique_ptr<const Expression> left,
                                     unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundGreater, "$0 > $1");
}

unique_ptr<const Expression>
GreaterOrEqual(unique_ptr<const Expression> left,
               unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundGreaterOrEqual, "$0 >= $1");
}

unique_ptr<const Expression> Less(unique_ptr<const Expression> left,
                                  unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundLess, "$0 < $1");
}

unique_ptr<const Expression> LessOrEqual(unique_ptr<const Expression> left,
                                         unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundLessOrEqual, "$0 <= $1");
}
} // namespace supersonic
