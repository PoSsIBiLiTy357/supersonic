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

#include "supersonic/cursor/core/compute.h"

#include "supersonic/utils/std_namespace.h"
#include "supersonic/utils/macros.h"
#include "supersonic/utils/exception/failureor.h"
#include "supersonic/base/exception/exception.h"
#include "supersonic/base/exception/exception_macros.h"
#include "supersonic/cursor/base/cursor.h"
#include "supersonic/cursor/proto/cursors.pb.h"
#include "supersonic/cursor/base/operation.h"
#include "supersonic/cursor/infrastructure/basic_cursor.h"
#include "supersonic/cursor/infrastructure/basic_operation.h"
#include "supersonic/expression/base/expression.h"

namespace supersonic {

class BufferAllocator;

namespace {

class ComputeCursor : public BasicCursor {
 public:
  ComputeCursor(unique_ptr<BoundExpressionTree> computation,
                rowcount_t row_capacity,
                unique_ptr<Cursor> child)
      : BasicCursor(computation->result_schema(), std::move(child)),
        row_capacity_(row_capacity),
        computation_(std::move(computation)) {}

  virtual ~ComputeCursor() {}

  virtual ResultView Next(rowcount_t max_row_count) {
    if (max_row_count > row_capacity_) max_row_count = row_capacity_;
    ResultView result = child()->Next(max_row_count);
    if (!result.has_data()) return result;
    EvaluationResult evaluated = computation_->Evaluate(result.view());
    PROPAGATE_ON_FAILURE(evaluated);
    return ResultView::Success(&evaluated.get());
  }

  virtual bool IsWaitingOnBarrierSupported() const { return true; }

  virtual CursorId GetCursorId() const { return COMPUTE; }

 private:
  const int row_capacity_;
  unique_ptr<BoundExpressionTree> computation_;
  DISALLOW_COPY_AND_ASSIGN(ComputeCursor);
};

class ComputeOperation : public BasicOperation {
 public:
  ComputeOperation(unique_ptr<const Expression> computation, unique_ptr<Operation> child)
      : BasicOperation(std::move(child)),
        computation_(std::move(computation)) {}

  virtual FailureOrOwned<Cursor> CreateCursor() const {
    FailureOrOwned<Cursor> bound_child(child()->CreateCursor());
    PROPAGATE_ON_FAILURE(bound_child);
    FailureOrOwned<BoundExpressionTree> bound_computation(
        computation_->Bind(bound_child->schema(),
                           buffer_allocator(),
                           default_row_count()));
    PROPAGATE_ON_FAILURE(bound_computation);
    return BoundCompute(bound_computation.move(), buffer_allocator(),
                        default_row_count(), bound_child.move());
  }

 private:
  const std::unique_ptr<const Expression> computation_;
};

}  // namespace

unique_ptr<Operation> Compute(unique_ptr<const Expression> computation,
                              unique_ptr<Operation> child) {
  return make_unique<ComputeOperation>(std::move(computation), std::move(child));
}

FailureOrOwned<Cursor> BoundCompute(unique_ptr<BoundExpressionTree> computation,
                                    BufferAllocator* const allocator,
                                    const rowcount_t max_row_count,
                                    unique_ptr<Cursor> child) {
  return Success(make_unique<ComputeCursor>(std::move(computation), max_row_count,
                                            std::move(child)));
}

}  // namespace supersonic
