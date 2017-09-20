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
// Mathematical functions.

#ifndef SUPERSONIC_EXPRESSION_CORE_MATH_EXPRESSIONS_H_
#define SUPERSONIC_EXPRESSION_CORE_MATH_EXPRESSIONS_H_

#include "supersonic/expression/base/expression.h"

namespace supersonic {

// Selected <math> functions.
//
// The *Signaling, *Nulling and *Quiet versions differ in their treatment
// of improper arguments (for instance of negative inputs to Sqrt or Log).
// The Signaling versions stop the evaluation with an error. The Nulling
// versions take the result to be NULL. The Quiet versions do what the
// underlying C++ functions do (so usually return a NaN, and generally behave
// according to IEE 754). If you do not care, as you trust your input data to
// be correct, choose the Quiet version, as it is the most efficient (the
// Nulling version tends to be least efficient).

// Exponent (e to the power argument).

unique_ptr<const Expression> Exp(unique_ptr<const Expression> const argument);
// TODO(onufry): add also a signaling version for the logarithms. If anybody
// needs those, please contact the Supersonic team, we'll add them ASAP.
// Various versions of logarithms - natural, base 10, base 2, and arbitrary
// base. The specialized versions are quicker and preferred to the arbitrary
// base one.
unique_ptr<const Expression> LnNulling(unique_ptr<const Expression> const argument);
unique_ptr<const Expression> LnQuiet(unique_ptr<const Expression> const argument);
unique_ptr<const Expression> Log10Nulling(unique_ptr<const Expression> const argument);
unique_ptr<const Expression> Log10Quiet(unique_ptr<const Expression> const argument);
unique_ptr<const Expression> Log2Nulling(unique_ptr<const Expression> const argument);
unique_ptr<const Expression> Log2Quiet(unique_ptr<const Expression> const argument);
unique_ptr<const Expression> LogNulling(unique_ptr<const Expression> base,
                             unique_ptr<const Expression> argument);
unique_ptr<const Expression> LogQuiet(unique_ptr<const Expression> base,
                           unique_ptr<const Expression> argument);

// Trigonometry.
unique_ptr<const Expression> Sin(unique_ptr<const Expression> const radians);
unique_ptr<const Expression> Cos(unique_ptr<const Expression> const radians);
unique_ptr<const Expression> Tan(unique_ptr<const Expression> const radians);
unique_ptr<const Expression> Cot(unique_ptr<const Expression> const radians);
// Arc trigonometry.
unique_ptr<const Expression> Asin(unique_ptr<const Expression> const argument);
unique_ptr<const Expression> Acos(unique_ptr<const Expression> const argument);
unique_ptr<const Expression> Atan(unique_ptr<const Expression> const argument);
unique_ptr<const Expression> Atan2(unique_ptr<const Expression> const x,
                        unique_ptr<const Expression> const y);
// Hyperbolic trigonometry.
unique_ptr<const Expression> Sinh(unique_ptr<const Expression> const argument);
unique_ptr<const Expression> Cosh(unique_ptr<const Expression> const argument);
unique_ptr<const Expression> Tanh(unique_ptr<const Expression> const argument);
// Hyperbolic arc trigonometry.
unique_ptr<const Expression> Asinh(unique_ptr<const Expression> const argument);
unique_ptr<const Expression> Acosh(unique_ptr<const Expression> const argument);
unique_ptr<const Expression> Atanh(unique_ptr<const Expression> const argument);
// Various others trigonometry-related.
unique_ptr<const Expression> ToDegrees(unique_ptr<const Expression> const radians);  // From radians.
unique_ptr<const Expression> ToRadians(unique_ptr<const Expression> const degrees);  // From degrees.
unique_ptr<const Expression> Pi();

// Absolute value.
unique_ptr<const Expression> Abs(unique_ptr<const Expression> const argument);
// The result type is equal to the input type.
unique_ptr<const Expression> Round(unique_ptr<const Expression> argument);
unique_ptr<const Expression> Ceil(unique_ptr<const Expression> const argument);
unique_ptr<const Expression> Floor(unique_ptr<const Expression> const argument);
unique_ptr<const Expression> Trunc(unique_ptr<const Expression> const argument);
// The result type is integer.
unique_ptr<const Expression> RoundToInt(unique_ptr<const Expression> argument);
unique_ptr<const Expression> CeilToInt(unique_ptr<const Expression> argument);
unique_ptr<const Expression> FloorToInt(unique_ptr<const Expression> argument);
// The result type is always double. The precision has to be an integer. Rounds
// up to precision decimal places. If precision is negative, the argument gets
// rounded to the nearest multiple of 1E-precision.
unique_ptr<const Expression> RoundWithPrecision(unique_ptr<const Expression> argument,
                                     unique_ptr<const Expression> precision);
// Square root. For the semantics of Signaling, Nulling and Quiet see notes at
// the beginning of the document.
unique_ptr<const Expression> SqrtSignaling(unique_ptr<const Expression> const argument);
unique_ptr<const Expression> SqrtNulling(unique_ptr<const Expression> const argument);
unique_ptr<const Expression> SqrtQuiet(unique_ptr<const Expression> const argument);

// Three versions of the Power function (base raised to the power exponent).
// The three versions differ in their treatment of incorrect arguments (to
// be more specific - their treatment of the case where the base is negative
// and the exponent is not an integer). See the comments at the beginning of
// the file for the descriptions of the policies.
// Note that all the versions assume that zero to the power zero is equal to
// one, and zero to negative powers is equal to infinity.
unique_ptr<const Expression> PowerSignaling(unique_ptr<const Expression> const base,
                                 unique_ptr<const Expression> const exponent);
unique_ptr<const Expression> PowerNulling(unique_ptr<const Expression> const base,
                               unique_ptr<const Expression> const exponent);
unique_ptr<const Expression> PowerQuiet(unique_ptr<const Expression> const base,
                             unique_ptr<const Expression> const exponent);


unique_ptr<const Expression> Format(unique_ptr<const Expression> const number,
                         unique_ptr<const Expression> const precision);

// Create bool expressions that compute the respective C99 classifications
// of floating point numbers.  The result is NULL if the argument is NULL.
// See http://www.opengroup.org/onlinepubs/000095399/functions/isfinite.html
// and, and it's SEE ALSO section.
// For integers is equivalent to the IsNull operator, but less effective.
unique_ptr<const Expression> IsFinite(unique_ptr<const Expression> number);
unique_ptr<const Expression> IsNormal(unique_ptr<const Expression> number);
unique_ptr<const Expression> IsNaN(unique_ptr<const Expression> number);
unique_ptr<const Expression> IsInf(unique_ptr<const Expression> number);

// Creates an expression of type DOUBLE that will, upon evaluation against
// a cursor, return a uniformly distributed pseudo random number in [0, 1].
unique_ptr<const Expression> RandomDouble();  // Not implemented.

// DEPRECATED, use the policy-conscious versions.
// instead.
unique_ptr<const Expression> Ln(unique_ptr<const Expression> const e);
unique_ptr<const Expression> Log10(unique_ptr<const Expression> const e);
unique_ptr<const Expression> Log(unique_ptr<const Expression> base, unique_ptr<const Expression> argument);
// DEPRECATED, use the policy-conscious version.
unique_ptr<const Expression> Sqrt(unique_ptr<const Expression> const e);

}  // namespace supersonic

#endif  // SUPERSONIC_EXPRESSION_CORE_MATH_EXPRESSIONS_H_
