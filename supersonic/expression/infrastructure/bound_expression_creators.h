// Copyright 2011 Google Inc.  All Rights Reserved
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
// Author: ptab@google.com (Piotr Tabor)
//
// Type definitions for standard bound expression factories (constructors)
// by arity.

#ifndef SUPERSONIC_EXPRESSION_INFRASTRUCTURE_BOUND_EXPRESSION_CREATORS_H_
#define SUPERSONIC_EXPRESSION_INFRASTRUCTURE_BOUND_EXPRESSION_CREATORS_H_

#include <cstddef>

#include "supersonic/base/exception/result.h"
#include "supersonic/base/infrastructure/types.h"
#include "supersonic/expression/base/expression.h"

namespace supersonic {

class BufferAllocator;

// Shorthands for function pointers to expression creation functions.
typedef unique_ptr<const Expression>(*ConstExpressionCreator)();

typedef unique_ptr<const Expression>(*UnaryExpressionCreator)(
    unique_ptr<const Expression>);

typedef unique_ptr<const Expression>(*BinaryExpressionCreator)(
    unique_ptr<const Expression>,
    unique_ptr<const Expression>);

typedef unique_ptr<const Expression>(*TernaryExpressionCreator)(
    unique_ptr<const Expression>,
    unique_ptr<const Expression>,
    unique_ptr<const Expression>);

typedef unique_ptr<const Expression>(*QuaternaryExpressionCreator)(
    unique_ptr<const Expression>,
    unique_ptr<const Expression>,
    unique_ptr<const Expression>,
    unique_ptr<const Expression>);

typedef unique_ptr<const Expression>(*QuinaryExpressionCreator)(
    unique_ptr<const Expression>,
    unique_ptr<const Expression>,
    unique_ptr<const Expression>,
    unique_ptr<const Expression>,
    unique_ptr<const Expression>);

typedef unique_ptr<const Expression>(*SenaryExpressionCreator)(
    unique_ptr<const Expression>,
    unique_ptr<const Expression>,
    unique_ptr<const Expression>,
    unique_ptr<const Expression>,
    unique_ptr<const Expression>,
    unique_ptr<const Expression>);

// TODO(ptab): Rename to *Creator (for example: BoundConstExpressionCreator).
typedef FailureOrOwned<BoundExpression> (*BoundConstExpressionFactory)(
    BufferAllocator *, rowcount_t);

typedef FailureOrOwned<BoundExpression> (*BoundUnaryExpressionFactory)(
    unique_ptr<BoundExpression>, BufferAllocator *, rowcount_t);

typedef FailureOrOwned<BoundExpression> (*BoundBinaryExpressionFactory)(
    unique_ptr<BoundExpression>, unique_ptr<BoundExpression>,
    BufferAllocator *, rowcount_t);

typedef FailureOrOwned<BoundExpression> (*BoundTernaryExpressionFactory)(
    unique_ptr<BoundExpression>, unique_ptr<BoundExpression>,
    unique_ptr<BoundExpression>, BufferAllocator *, rowcount_t);

typedef FailureOrOwned<BoundExpression> (*BoundQuaternaryExpressionFactory)(
    unique_ptr<BoundExpression>, unique_ptr<BoundExpression>,
    unique_ptr<BoundExpression>, unique_ptr<BoundExpression>,
    BufferAllocator *, rowcount_t);

typedef FailureOrOwned<BoundExpression> (*BoundSenaryExpressionFactory)(
    unique_ptr<BoundExpression>, unique_ptr<BoundExpression>,
    unique_ptr<BoundExpression>, unique_ptr<BoundExpression>,
    unique_ptr<BoundExpression>, unique_ptr<BoundExpression>,
    BufferAllocator *, rowcount_t);

typedef FailureOrOwned<BoundExpression> (*BoundExpressionListExpressionFactory)(
    unique_ptr<BoundExpressionList>, BufferAllocator *, rowcount_t);

}  // namespace supersonic
#endif  // SUPERSONIC_EXPRESSION_INFRASTRUCTURE_BOUND_EXPRESSION_CREATORS_H_
