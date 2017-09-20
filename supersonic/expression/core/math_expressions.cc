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


#include <cmath>

#include "supersonic/expression/core/math_bound_expressions.h"
#include "supersonic/expression/core/math_evaluators.h" // IWYU pragma: keep
#include "supersonic/expression/infrastructure/basic_expressions.h"
#include "supersonic/expression/infrastructure/terminal_expressions.h"
#include "supersonic/expression/core/math_expressions.h"

namespace supersonic {

// Exponent and logarithm.
class Expression;

unique_ptr<const Expression> Exp(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument), &BoundExp,
                                                 "EXP($0)");
}

unique_ptr<const Expression> LnNulling(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundLnNulling, "LN($0)");
}

unique_ptr<const Expression> LnQuiet(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundLnQuiet, "LN($0)");
}

unique_ptr<const Expression>
Log10Nulling(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(
      std::move(argument), &BoundLog10Nulling, "LOG10($0)");
}

unique_ptr<const Expression> Log10Quiet(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundLog10Quiet, "LOG10($0)");
}

unique_ptr<const Expression>
Log2Nulling(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundLog2Nulling, "LOG2($0)");
}

unique_ptr<const Expression> Log2Quiet(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundLog2Quiet, "LOG2($0)");
}

unique_ptr<const Expression> LogNulling(unique_ptr<const Expression> base,
                                        unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(
      std::move(base), std::move(argument), &BoundLogNulling, "LOG($0, $1)");
}

unique_ptr<const Expression> LogQuiet(unique_ptr<const Expression> base,
                                      unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(
      std::move(base), std::move(argument), &BoundLogQuiet, "LOG($0, $1)");
}

// Absolute value.
unique_ptr<const Expression> Abs(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument), &BoundAbs,
                                                 "ABS($0)");
}

// Various rounding functions.
unique_ptr<const Expression> Floor(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundFloor, "FLOOR($0)");
}

unique_ptr<const Expression> Ceil(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundCeil, "CEIL($0)");
}

unique_ptr<const Expression> CeilToInt(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(
      std::move(argument), &BoundCeilToInt, "CEIL_TO_INT($0)");
}

unique_ptr<const Expression> FloorToInt(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(
      std::move(argument), &BoundFloorToInt, "FLOOR_TO_INT($0)");
}

unique_ptr<const Expression> Trunc(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundTrunc, "TRUNC($0)");
}

unique_ptr<const Expression> Round(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundRound, "ROUND($0)");
}

unique_ptr<const Expression> RoundToInt(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(
      std::move(argument), &BoundRoundToInt, "ROUND_TO_INT($0)");
}

unique_ptr<const Expression>
RoundWithPrecision(unique_ptr<const Expression> argument,
                   unique_ptr<const Expression> precision) {
  return CreateExpressionForExistingBoundFactory(
      std::move(argument), std::move(precision), &BoundRoundWithPrecision,
      "ROUND_WITH_PRECISION($0, $1)");
}

// Square root.
unique_ptr<const Expression>
SqrtSignaling(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(
      std::move(argument), &BoundSqrtSignaling, "SQRT($0)");
}

unique_ptr<const Expression>
SqrtNulling(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundSqrtNulling, "SQRT($0)");
}

unique_ptr<const Expression> SqrtQuiet(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundSqrtQuiet, "SQRT($0)");
}

// Power.
unique_ptr<const Expression>
PowerSignaling(unique_ptr<const Expression> base,
               unique_ptr<const Expression> exponent) {
  return CreateExpressionForExistingBoundFactory(
      std::move(base), std::move(exponent), &BoundPowerSignaling,
      "POW($0, $1)");
}

unique_ptr<const Expression>
PowerNulling(unique_ptr<const Expression> base,
             unique_ptr<const Expression> exponent) {
  return CreateExpressionForExistingBoundFactory(
      std::move(base), std::move(exponent), &BoundPowerNulling, "POW($0, $1)");
}

unique_ptr<const Expression>
PowerQuiet(unique_ptr<const Expression> base,
           unique_ptr<const Expression> exponent) {
  return CreateExpressionForExistingBoundFactory(
      std::move(base), std::move(exponent), &BoundPowerQuiet, "POW($0, $1)");
}

// Trigonometry.
unique_ptr<const Expression> Sin(unique_ptr<const Expression> radians) {
  return CreateExpressionForExistingBoundFactory(std::move(radians), &BoundSin,
                                                 "SIN($0)");
}

unique_ptr<const Expression> Cos(unique_ptr<const Expression> radians) {
  return CreateExpressionForExistingBoundFactory(std::move(radians), &BoundCos,
                                                 "COS($0)");
}

unique_ptr<const Expression> Tan(unique_ptr<const Expression> radians) {
  return CreateExpressionForExistingBoundFactory(std::move(radians),
                                                 &BoundTanQuiet, "TAN($0)");
}

unique_ptr<const Expression> Cot(unique_ptr<const Expression> radians) {
  return CreateExpressionForExistingBoundFactory(std::move(radians), &BoundCot,
                                                 "COT($0)");
}

unique_ptr<const Expression> Asin(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundAsin, "ASIN($0)");
}

unique_ptr<const Expression> Acos(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundAcos, "ACOS($0)");
}

unique_ptr<const Expression> Atan(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundAtan, "ATAN($0)");
}

unique_ptr<const Expression> Atan2(unique_ptr<const Expression> x,
                                   unique_ptr<const Expression> y) {
  return CreateExpressionForExistingBoundFactory(std::move(x), std::move(y),
                                                 &BoundAtan2, "ATAN2($0, $1)");
}

unique_ptr<const Expression> Sinh(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundSinh, "SINH($0)");
}

unique_ptr<const Expression> Cosh(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundCosh, "COSH($0)");
}

unique_ptr<const Expression> Tanh(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundTanh, "TANH($0)");
}

unique_ptr<const Expression> Asinh(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundAsinh, "ASINH($0)");
}

unique_ptr<const Expression> Acosh(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundAcosh, "ACOSH($0)");
}

unique_ptr<const Expression> Atanh(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(std::move(argument),
                                                 &BoundAtanh, "ATANH($0)");
}

unique_ptr<const Expression> ToDegrees(unique_ptr<const Expression> radians) {
  return CreateExpressionForExistingBoundFactory(
      std::move(radians), &BoundToDegrees, "DEGREES($0)");
}

unique_ptr<const Expression> ToRadians(unique_ptr<const Expression> degrees) {
  return CreateExpressionForExistingBoundFactory(
      std::move(degrees), &BoundToRadians, "RADIANS($0)");
}

unique_ptr<const Expression> Pi() { return ConstDouble(M_PI); }

// IEEE checks.
unique_ptr<const Expression> IsFinite(unique_ptr<const Expression> number) {
  return CreateExpressionForExistingBoundFactory(
      std::move(number), &BoundIsFinite, "IS_FINITE($0)");
}

unique_ptr<const Expression> IsInf(unique_ptr<const Expression> number) {
  return CreateExpressionForExistingBoundFactory(std::move(number), &BoundIsInf,
                                                 "IS_INF($0)");
}

unique_ptr<const Expression> IsNaN(unique_ptr<const Expression> number) {
  return CreateExpressionForExistingBoundFactory(std::move(number), &BoundIsNaN,
                                                 "IS_NAN($0)");
}

unique_ptr<const Expression> IsNormal(unique_ptr<const Expression> number) {
  return CreateExpressionForExistingBoundFactory(
      std::move(number), &BoundIsNormal, "IS_NORMAL($0)");
}

// Other.
unique_ptr<const Expression> Format(unique_ptr<const Expression> number,
                                    unique_ptr<const Expression> precision) {
  return CreateExpressionForExistingBoundFactory(
      std::move(number), std::move(precision), &BoundFormatSignaling,
      "FORMAT($0, $1)");
}

// Deprecated.
unique_ptr<const Expression> Sqrt(unique_ptr<const Expression> e) {
  return SqrtQuiet(std::move(e));
}

unique_ptr<const Expression> Ln(unique_ptr<const Expression> e) {
  return LnNulling(std::move(e));
}

unique_ptr<const Expression> Log10(unique_ptr<const Expression> e) {
  return Log10Nulling(std::move(e));
}

unique_ptr<const Expression> Log(unique_ptr<const Expression> base,
                                 unique_ptr<const Expression> argument) {
  return LogNulling(std::move(base), std::move(argument));
}

} // namespace supersonic
