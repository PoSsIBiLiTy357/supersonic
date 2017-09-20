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
//
// Basic arithmetic expressions.

#ifndef SUPERSONIC_EXPRESSION_CORE_ARITHMETIC_EXPRESSIONS_H_
#define SUPERSONIC_EXPRESSION_CORE_ARITHMETIC_EXPRESSIONS_H_

#include "supersonic/utils/std_namespace.h"
#include "supersonic/expression/base/expression.h"


namespace supersonic {

// Floating point arithmetics is handled according to IEEE 754, i.e.
// infinite and NaN values can result.

// Creates an expression that will return a sum of two subexpressions.
class Expression;

unique_ptr<const Expression> Plus(unique_ptr<const Expression> a,
                       unique_ptr<const Expression> b);

// Creates an expression that will return a difference of two subexpressions.
unique_ptr<const Expression> Minus(unique_ptr<const Expression> a,
                        unique_ptr<const Expression> b);

// Creates an expression that will return a product of two subexpressions.
unique_ptr<const Expression> Multiply(unique_ptr<const Expression> a,
                           unique_ptr<const Expression> b);

// Creates an expression that will return a ratio of two subexpressions.
//
// DEPRECATED. Use one of the policy-conscious types instead.
unique_ptr<const Expression> Divide(unique_ptr<const Expression> a,
                         unique_ptr<const Expression> b);

// Creates an expression that will return a ratio of two subexpressions. Will
// fail the evaluation at division by zero.
unique_ptr<const Expression> DivideSignaling(unique_ptr<const Expression> a,
                                  unique_ptr<const Expression> b);

// This version assumes x / 0 = NULL.
unique_ptr<const Expression> DivideNulling(unique_ptr<const Expression> a,
                                unique_ptr<const Expression> b);

// This version assumes division as in CPP (so x / 0 is an inf, -inf or nan,
// depending on x).
unique_ptr<const Expression> DivideQuiet(unique_ptr<const Expression> a,
                              unique_ptr<const Expression> b);

// TODO(onufry): This expression should be removed in favour of an
// IntegerDivide, which will take only integer arguments.
// Creates an expression that will return the division result (rounded in case
// of integers). Examples: 5 / 2 = 2, but 5.0 / 2 = 2.5 and 5 / 2.0 = 2.5.
//
// DEPRECATED! Use one of the policy-concious types instead.
unique_ptr<const Expression> CppDivide(unique_ptr<const Expression> a,
                            unique_ptr<const Expression> b);

// This version assumes x / 0 = NULL.
unique_ptr<const Expression> CppDivideNulling(unique_ptr<const Expression> a,
                                   unique_ptr<const Expression> b);

// This version assumes x / 0 fails.
unique_ptr<const Expression> CppDivideSignaling(unique_ptr<const Expression> a,
                                     unique_ptr<const Expression> b);

// Creates an expression that negates a number.
unique_ptr<const Expression> Negate(unique_ptr<const Expression> a);

// Creates an expression that will return a modulus of two integer
// subexpressions.
unique_ptr<const Expression> Modulus(unique_ptr<const Expression> a,
                          unique_ptr<const Expression> b);

// This version assumes x % 0 == NULL.
unique_ptr<const Expression> ModulusNulling(unique_ptr<const Expression> a,
                                 unique_ptr<const Expression> b);

// This version assumes x % 0 fails.
unique_ptr<const Expression> ModulusSignaling(unique_ptr<const Expression> a,
                                   unique_ptr<const Expression> b);

// Creates an expression that will return true if the argument is odd.
// Requires the argument to be integer.
unique_ptr<const Expression> IsOdd(unique_ptr<const Expression> arg);

// Creates an expression that will return true if the argument is even.
// Requires the argument to be integer.
unique_ptr<const Expression> IsEven(unique_ptr<const Expression> arg);

}  // namespace supersonic

#endif  // SUPERSONIC_EXPRESSION_CORE_ARITHMETIC_EXPRESSIONS_H_
