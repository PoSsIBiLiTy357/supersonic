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
// Author:  onufry@google.com (Jakub Onufry Wojtaszczyk)

#include "supersonic/expression/core/arithmetic_expressions.h"

#include "supersonic/expression/core/arithmetic_bound_expressions.h"
#include "supersonic/expression/infrastructure/basic_expressions.h"

namespace supersonic {

// TODO(onufry): We should add a description<OperatorId>() function that would
// return the appropriate string of the ($0 + $1) type. This should go through
// traits, and in this way we would declare the description in a single place
// only.

// Expressions instantiation:
class Expression;

unique_ptr<const Expression> Plus(unique_ptr<const Expression> left,
                                  unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundPlus, "($0 + $1)");
}

unique_ptr<const Expression> Minus(unique_ptr<const Expression> left,
                                   unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundMinus, "($0 - $1)");
}

unique_ptr<const Expression> Multiply(unique_ptr<const Expression> left,
                                      unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundMultiply, "($0 * $1)");
}

unique_ptr<const Expression>
DivideSignaling(unique_ptr<const Expression> left,
                unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundDivideSignaling, "($0 /. $1)");
}

unique_ptr<const Expression> DivideNulling(unique_ptr<const Expression> left,
                                           unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundDivideNulling, "($0 /. $1)");
}

unique_ptr<const Expression> DivideQuiet(unique_ptr<const Expression> left,
                                         unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundDivideQuiet, "($0 /. $1)");
}

unique_ptr<const Expression>
CppDivideSignaling(unique_ptr<const Expression> left,
                   unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundCppDivideSignaling, "($0 / $1)");
}

unique_ptr<const Expression>
CppDivideNulling(unique_ptr<const Expression> left,
                 unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundCppDivideNulling, "($0 / $1)");
}

unique_ptr<const Expression> Negate(unique_ptr<const Expression> child) {
  return CreateExpressionForExistingBoundFactory(std::move(child), &BoundNegate,
                                                 "(-$0)");
}

unique_ptr<const Expression>
ModulusSignaling(unique_ptr<const Expression> left,
                 unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundModulusSignaling, "$0 % $1");
}

unique_ptr<const Expression>
ModulusNulling(unique_ptr<const Expression> left,
               unique_ptr<const Expression> right) {
  return CreateExpressionForExistingBoundFactory(
      std::move(left), std::move(right), &BoundModulusNulling, "$0 % $1");
}

// TODO(onufry): Delete these, in favor of policy-specific functions, and
// refactor our clients not to use them.
unique_ptr<const Expression> Modulus(unique_ptr<const Expression> left,
                                     unique_ptr<const Expression> right) {
  return ModulusSignaling(std::move(left), std::move(right));
}

unique_ptr<const Expression> Divide(unique_ptr<const Expression> left,
                                    unique_ptr<const Expression> right) {
  return DivideSignaling(std::move(left), std::move(right));
}

unique_ptr<const Expression> CppDivide(unique_ptr<const Expression> left,
                                       unique_ptr<const Expression> right) {
  return CppDivideSignaling(std::move(left), std::move(right));
}

}  // namespace supersonic
