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


#include <glog/logging.h>
#include "supersonic/utils/logging-inl.h"
#include "supersonic/utils/macros.h"
#include "supersonic/utils/exception/failureor.h"
#include "supersonic/base/exception/exception.h"
#include "supersonic/base/exception/exception_macros.h"
#include "supersonic/base/infrastructure/block.h"
#include "supersonic/base/infrastructure/projector.h"
#include "supersonic/base/infrastructure/types.h"
#include "supersonic/cursor/base/cursor.h"
#include "supersonic/cursor/base/operation.h"
#include "supersonic/cursor/proto/cursors.pb.h"
#include "supersonic/cursor/core/generate.h"
#include "supersonic/cursor/infrastructure/basic_cursor.h"
#include "supersonic/cursor/infrastructure/basic_operation.h"
#include "supersonic/cursor/infrastructure/iterators.h"
#include "supersonic/utils/pointer_vector.h"
#include "supersonic/utils/stl_util.h"
#include "supersonic/cursor/core/coalesce.h"

namespace supersonic {

class TupleSchema;

namespace {

class CoalesceCursor: public BasicCursor {
 using Container = CursorIterator;

 public:
  // Takes ownership of child cursors and the projector
  // The child cursors must not be NULL and must have distinct attribute names.
  CoalesceCursor(vector<unique_ptr<Cursor>> children,
                 unique_ptr<const BoundMultiSourceProjector> projector)
      : BasicCursor(projector->result_schema()),
        projector_(std::move(projector)) {
    for (auto& child: children) {
      CHECK_NOTNULL(projector_.get());
      DCHECK(child != nullptr);
      inputs_.emplace_back(make_unique<Container>(std::move(child)));
    }
  }

  virtual ResultView Next(rowcount_t max_row_count);
  virtual bool IsWaitingOnBarrierSupported() const { return true; }

  virtual void ApplyToChildren(CursorTransformer* transformer) {
    for (auto& input: inputs_) {
      input->ApplyToCursor(transformer);
    }
  }

  virtual CursorId GetCursorId() const { return COALESCE; }

 private:
  vector<unique_ptr<Container>> inputs_;
  std::unique_ptr<const BoundMultiSourceProjector> projector_;

  DISALLOW_COPY_AND_ASSIGN(CoalesceCursor);
};

ResultView CoalesceCursor::Next(rowcount_t max_row_count) {
  const int inputs_count = inputs_.size();
  DCHECK_GT(inputs_count, 0);
  for (int i = 0; i < inputs_count; ++i) {
    CursorIterator* input = inputs_[i].get();
    if (!input->Next(max_row_count, false)) {
      for (int j = 0; j < i; ++j) {
        inputs_[j]->truncate(0);
      }
      return input->result();
    }
    const int view_row_count = input->view().row_count();
    DCHECK_GT(view_row_count, 0);
    if (view_row_count < max_row_count) {
      max_row_count = view_row_count;
    }
  }
  vector<const View*> views;
  for (int i = 0; i < inputs_count; ++i) {
    CursorIterator* input = inputs_[i].get();
    input->truncate(max_row_count);
    views.push_back(&input->view());
  }
  projector_->Project(views.begin(), views.end(), my_view());
  my_view()->set_row_count(max_row_count);
  return ResultView::Success(my_view());
}

class CoalesceOperation : public BasicOperation {
 public:
  virtual ~CoalesceOperation() {}

  explicit CoalesceOperation(vector<unique_ptr<Operation>> children)
      : BasicOperation(std::move(children)) {}

  virtual FailureOrOwned<Cursor> CreateCursor() const {
    vector<unique_ptr<Cursor>> child_cursors(children_count());
    for (int i = 0; i < children_count(); ++i) {
      FailureOrOwned<Cursor> child_cursor = child_at(i)->CreateCursor();
      PROPAGATE_ON_FAILURE(child_cursor);
      child_cursors[i] = child_cursor.move();
    }
    FailureOrOwned<Cursor> cursor = BoundCoalesce(std::move(child_cursors));
    PROPAGATE_ON_FAILURE(cursor);
    return Success(cursor.move());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CoalesceOperation);
};

}  // namespace

FailureOrOwned<Cursor> BoundCoalesce(vector<unique_ptr<Cursor>> children) {
  if (children.empty()) {
    return BoundGenerate(0);
  }
  if (children.size() == 1) {
    return Success(std::move(children[0]));
  }

  CompoundMultiSourceProjector all_attributes_projector;
  vector<TupleSchema> child_schemas;
  for (int i = 0; i < children.size(); ++i) {
    Cursor* child = children[i].get();
    DCHECK(child != NULL);
    child_schemas.emplace_back(child->schema());
    all_attributes_projector.add(i, ProjectAllAttributes());
  }
  FailureOrOwned<const BoundMultiSourceProjector> bound_projector =
      all_attributes_projector.Bind(child_schemas);
  PROPAGATE_ON_FAILURE(bound_projector);
  return Success(make_unique<CoalesceCursor>(std::move(children), bound_projector.move()));
}

unique_ptr<Operation> Coalesce(vector<unique_ptr<Operation>> children) {
  return make_unique<CoalesceOperation>(std::move(children));
}

}  // namespace supersonic
