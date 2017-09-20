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

#include "supersonic/cursor/core/project.h"

#include <glog/logging.h>
#include "supersonic/utils/std_namespace.h"
#include "supersonic/utils/logging-inl.h"
#include "supersonic/utils/macros.h"
#include "supersonic/utils/exception/failureor.h"
#include "supersonic/base/exception/exception.h"
#include "supersonic/base/exception/exception_macros.h"
#include "supersonic/base/exception/result.h"
#include "supersonic/base/infrastructure/block.h"
#include "supersonic/base/infrastructure/projector.h"
#include "supersonic/base/infrastructure/types.h"
#include "supersonic/cursor/base/cursor.h"
#include "supersonic/cursor/proto/cursors.pb.h"
#include "supersonic/cursor/base/operation.h"
#include "supersonic/cursor/infrastructure/basic_cursor.h"
#include "supersonic/cursor/infrastructure/basic_operation.h"

namespace supersonic {

namespace {

class ProjectCursor : public BasicCursor {
 public:
  // Takes ownership of BoundSingleSourceProjector and child cursor.
  ProjectCursor(unique_ptr<const BoundSingleSourceProjector> projector, unique_ptr<Cursor> child)
      : BasicCursor(projector->result_schema(), std::move(child)),
        projector_(std::move(projector)),
        result_view_(projector_->result_schema()) {}

  virtual ResultView Next(rowcount_t max_row_count) {
    ResultView next_result = child()->Next(max_row_count);
    PROPAGATE_ON_FAILURE(next_result);
    if (!next_result.has_data()) {
      CHECK(next_result.is_eos() || next_result.is_waiting_on_barrier());
      return next_result;
    }
    projector_->Project(next_result.view(), &result_view_);
    result_view_.set_row_count(next_result.view().row_count());
    return ResultView::Success(&result_view_);
  }

  virtual bool IsWaitingOnBarrierSupported() const { return true; }

  virtual CursorId GetCursorId() const { return PROJECT; }

 private:
  unique_ptr<const BoundSingleSourceProjector> projector_;
  View result_view_;

  DISALLOW_COPY_AND_ASSIGN(ProjectCursor);
};

class ProjectOperation : public BasicOperation {
 public:
  // Takes ownership of projector and child_operation.
  ProjectOperation(unique_ptr<const SingleSourceProjector> projector,
                   unique_ptr<Operation> child_operation)
      : BasicOperation(std::move(child_operation)),
        projector_(std::move(projector)) {}

  virtual ~ProjectOperation() {}

  virtual FailureOrOwned<Cursor> CreateCursor() const {
    FailureOrOwned<Cursor> child_cursor = child()->CreateCursor();
    PROPAGATE_ON_FAILURE(child_cursor);
    FailureOrOwned<const BoundSingleSourceProjector> bound_projector =
        projector_->Bind(child_cursor->schema());
    PROPAGATE_ON_FAILURE(bound_projector);
    return Success(
        BoundProject(bound_projector.move(), child_cursor.move()));
  }

 private:
  unique_ptr<const SingleSourceProjector> projector_;

  DISALLOW_COPY_AND_ASSIGN(ProjectOperation);
};

}  // namespace

unique_ptr<Operation> Project(unique_ptr<const SingleSourceProjector> projector,
                              unique_ptr<Operation> child) {
  return make_unique<ProjectOperation>(std::move(projector), std::move(child));
}

unique_ptr<Cursor> BoundProject(unique_ptr<const BoundSingleSourceProjector> projector,
                                unique_ptr<Cursor> child) {
  return make_unique<ProjectCursor>(std::move(projector), std::move(child));
}

}  // namespace supersonic
