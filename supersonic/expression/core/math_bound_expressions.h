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
//
// Bound expression accessors for math functions.i
// For the descriptions of particular functions see math_expressions.h.
// For the usage of bound expression accessors, see expression.h or kick the
// Supersonic team for documentation.

#ifndef SUPERSONIC_EXPRESSION_CORE_MATH_BOUND_EXPRESSIONS_H_
#define SUPERSONIC_EXPRESSION_CORE_MATH_BOUND_EXPRESSIONS_H_

#include "supersonic/base/exception/result.h"
#include "supersonic/base/infrastructure/types.h"

namespace supersonic {

// ---------------------- Exponents, logarithms, powers ------------------------

class BoundExpression;
class BufferAllocator;

FailureOrOwned<BoundExpression> BoundExp(unique_ptr<BoundExpression> arg,
                                         BufferAllocator* allocator,
                                         rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundLnNulling(unique_ptr<BoundExpression> arg,
                                               BufferAllocator* allocator,
                                               rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundLnQuiet(unique_ptr<BoundExpression> arg,
                                             BufferAllocator* allocator,
                                             rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundLog10Nulling(unique_ptr<BoundExpression> arg,
                                                  BufferAllocator* allocator,
                                                  rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundLog10Quiet(unique_ptr<BoundExpression> arg,
                                                BufferAllocator* allocator,
                                                rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundLogNulling(unique_ptr<BoundExpression> base,
                                                unique_ptr<BoundExpression> argument,
                                                BufferAllocator* allocator,
                                                rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundLogQuiet(unique_ptr<BoundExpression> base,
                                              unique_ptr<BoundExpression> argument,
                                              BufferAllocator* allocator,
                                              rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundLog2Nulling(unique_ptr<BoundExpression> arg,
                                                 BufferAllocator* allocator,
                                                 rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundLog2Quiet(unique_ptr<BoundExpression> arg,
                                               BufferAllocator* allocator,
                                               rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundPowerSignaling(unique_ptr<BoundExpression> base,
                                                    unique_ptr<BoundExpression> exponent,
                                                    BufferAllocator* allocator,
                                                    rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundPowerNulling(unique_ptr<BoundExpression> base,
                                                  unique_ptr<BoundExpression> exponent,
                                                  BufferAllocator* allocator,
                                                  rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundPowerQuiet(unique_ptr<BoundExpression> base,
                                                unique_ptr<BoundExpression> exponent,
                                                BufferAllocator* allocator,
                                                rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundSqrtSignaling(unique_ptr<BoundExpression> arg,
                                                   BufferAllocator* allocator,
                                                   rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundSqrtNulling(unique_ptr<BoundExpression> arg,
                                                 BufferAllocator* allocator,
                                                 rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundSqrtQuiet(unique_ptr<BoundExpression> arg,
                                               BufferAllocator* allocator,
                                               rowcount_t max_row_count);

// ----------------------------- Trigonometry ----------------------------------

FailureOrOwned<BoundExpression> BoundSin(unique_ptr<BoundExpression> arg,
                                         BufferAllocator* allocator,
                                         rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundCos(unique_ptr<BoundExpression> arg,
                                         BufferAllocator* allocator,
                                         rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundTanQuiet(unique_ptr<BoundExpression> arg,
                                              BufferAllocator* allocator,
                                              rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundCot(unique_ptr<BoundExpression> arg,
                                         BufferAllocator* allocator,
                                         rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundAsin(unique_ptr<BoundExpression> arg,
                                          BufferAllocator* allocator,
                                          rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundAcos(unique_ptr<BoundExpression> arg,
                                          BufferAllocator* allocator,
                                          rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundAtan(unique_ptr<BoundExpression> arg,
                                          BufferAllocator* allocator,
                                          rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundAtan2(unique_ptr<BoundExpression> x,
                                           unique_ptr<BoundExpression> y,
                                           BufferAllocator* allocator,
                                           rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundSinh(unique_ptr<BoundExpression> arg,
                                          BufferAllocator* allocator,
                                          rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundCosh(unique_ptr<BoundExpression> arg,
                                          BufferAllocator* allocator,
                                          rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundTanh(unique_ptr<BoundExpression> arg,
                                          BufferAllocator* allocator,
                                          rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundAsinh(unique_ptr<BoundExpression> arg,
                                           BufferAllocator* allocator,
                                           rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundAcosh(unique_ptr<BoundExpression> arg,
                                           BufferAllocator* allocator,
                                           rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundAtanh(unique_ptr<BoundExpression> arg,
                                           BufferAllocator* allocator,
                                           rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundToDegrees(unique_ptr<BoundExpression> arg,
                                               BufferAllocator* allocator,
                                               rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundToRadians(unique_ptr<BoundExpression> arg,
                                               BufferAllocator* allocator,
                                               rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundPi(BufferAllocator* allocator,
                                        rowcount_t max_row_count);

// ------------------------------------ Rounding -------------------------------

FailureOrOwned<BoundExpression> BoundRound(unique_ptr<BoundExpression> arg,
                                           BufferAllocator* allocator,
                                           rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundRoundToInt(unique_ptr<BoundExpression> arg,
                                                BufferAllocator* allocator,
                                                rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundRoundWithPrecision(
    unique_ptr<BoundExpression> argument,
    unique_ptr<BoundExpression> precision,
    BufferAllocator* allocator,
    rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundFloor(unique_ptr<BoundExpression> arg,
                                           BufferAllocator* allocator,
                                           rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundFloorToInt(unique_ptr<BoundExpression> arg,
                                                BufferAllocator* allocator,
                                                rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundCeil(unique_ptr<BoundExpression> arg,
                                          BufferAllocator* allocator,
                                          rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundCeilToInt(unique_ptr<BoundExpression> arg,
                                               BufferAllocator* allocator,
                                               rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundTrunc(unique_ptr<BoundExpression> arg,
                                           BufferAllocator* allocator,
                                           rowcount_t max_row_count);

// -------------------------------- IEEE 754 checks ----------------------------

FailureOrOwned<BoundExpression> BoundIsFinite(unique_ptr<BoundExpression> arg,
                                              BufferAllocator* allocator,
                                              rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundIsNormal(unique_ptr<BoundExpression> arg,
                                              BufferAllocator* allocator,
                                              rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundIsNaN(unique_ptr<BoundExpression> arg,
                                           BufferAllocator* allocator,
                                           rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundIsInf(unique_ptr<BoundExpression> arg,
                                           BufferAllocator* allocator,
                                           rowcount_t max_row_count);

// ------------------------------------- Other ---------------------------------

FailureOrOwned<BoundExpression> BoundAbs(unique_ptr<BoundExpression> argument,
                                         BufferAllocator* allocator,
                                         rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundFormatSignaling(unique_ptr<BoundExpression> number,
                                                     unique_ptr<BoundExpression> precision,
                                                     BufferAllocator* allocator,
                                                     rowcount_t max_row_count);

}  // namespace supersonic


#endif  // SUPERSONIC_EXPRESSION_CORE_MATH_BOUND_EXPRESSIONS_H_
