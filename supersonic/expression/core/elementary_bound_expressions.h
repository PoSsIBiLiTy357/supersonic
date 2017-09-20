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
// Elementary expressions: logical, relations, control expressions.
// For comments on the semantics see elementary_expressions.h.

#ifndef SUPERSONIC_EXPRESSION_CORE_ELEMENTARY_BOUND_EXPRESSIONS_H_
#define SUPERSONIC_EXPRESSION_CORE_ELEMENTARY_BOUND_EXPRESSIONS_H_

#include "supersonic/base/exception/result.h"
#include "supersonic/base/infrastructure/types.h"
#include "supersonic/proto/supersonic.pb.h"

namespace supersonic {

// --------------- Control expressions -----------------------------------------

class BoundExpression;
class BoundExpressionList;
class BufferAllocator;

FailureOrOwned<BoundExpression> BoundCastTo(DataType to_type,
                                            unique_ptr<BoundExpression> source,
                                            BufferAllocator* allocator,
                                            rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundParseStringQuiet(
    DataType to_type,
    unique_ptr<BoundExpression> source,
    BufferAllocator* allocator,
    rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundParseStringNulling(
    DataType to_type,
    unique_ptr<BoundExpression> source,
    BufferAllocator* allocator,
    rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundIfNull(unique_ptr<BoundExpression> e,
                                            unique_ptr<BoundExpression> substitute,
                                            BufferAllocator* allocator,
                                            rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundCase(unique_ptr<BoundExpressionList> bound_arguments,
                                          BufferAllocator* allocator,
                                          rowcount_t max_row_count);

// Returns "otherwise" if condition is NULL
FailureOrOwned<BoundExpression> BoundIf(unique_ptr<BoundExpression> condition,
                                        unique_ptr<BoundExpression> then,
                                        unique_ptr<BoundExpression> otherwise,
                                        BufferAllocator* allocator,
                                        rowcount_t max_row_count);

// Returns NULL if condition is NULL.
FailureOrOwned<BoundExpression> BoundIfNulling(unique_ptr<BoundExpression> condition,
                                               unique_ptr<BoundExpression> if_true,
                                               unique_ptr<BoundExpression> if_false,
                                               BufferAllocator* allocator,
                                               rowcount_t max_row_count);

// --------------- Logic -------------------------------------------------------

FailureOrOwned<BoundExpression> BoundNot(unique_ptr<BoundExpression> source,
                                         BufferAllocator* allocator,
                                         rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundOr(unique_ptr<BoundExpression> left,
                                        unique_ptr<BoundExpression> right,
                                        BufferAllocator* allocator,
                                        rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundAnd(unique_ptr<BoundExpression> left,
                                         unique_ptr<BoundExpression> right,
                                         BufferAllocator* allocator,
                                         rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundAndNot(unique_ptr<BoundExpression> left,
                                            unique_ptr<BoundExpression> right,
                                            BufferAllocator* allocator,
                                            rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundXor(unique_ptr<BoundExpression> left,
                                         unique_ptr<BoundExpression> right,
                                         BufferAllocator* allocator,
                                         rowcount_t max_row_count);

// --------------- Unary comparisions and checks -------------------------------

FailureOrOwned<BoundExpression> BoundIsNull(unique_ptr<BoundExpression> source,
                                            BufferAllocator* allocator,
                                            rowcount_t max_row_count);

// --------------- Bitwise operators -------------------------------------------

FailureOrOwned<BoundExpression> BoundBitwiseNot(unique_ptr<BoundExpression> argument,
                                                BufferAllocator* allocator,
                                                rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundBitwiseAnd(unique_ptr<BoundExpression> left,
                                                unique_ptr<BoundExpression> right,
                                                BufferAllocator* allocator,
                                                rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundBitwiseAndNot(unique_ptr<BoundExpression> left,
                                                   unique_ptr<BoundExpression> right,
                                                   BufferAllocator* allocator,
                                                   rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundBitwiseOr(unique_ptr<BoundExpression> left,
                                               unique_ptr<BoundExpression> right,
                                               BufferAllocator* allocator,
                                               rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundBitwiseXor(unique_ptr<BoundExpression> left,
                                                unique_ptr<BoundExpression> right,
                                                BufferAllocator* allocator,
                                                rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundShiftLeft(unique_ptr<BoundExpression> argument,
                                               unique_ptr<BoundExpression> shift,
                                               BufferAllocator* allocator,
                                               rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundShiftRight(unique_ptr<BoundExpression> argument,
                                                unique_ptr<BoundExpression> shift,
                                                BufferAllocator* allocator,
                                                rowcount_t max_row_count);

}  // namespace supersonic

#endif  // SUPERSONIC_EXPRESSION_CORE_ELEMENTARY_BOUND_EXPRESSIONS_H_
