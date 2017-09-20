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
//
// Cursor wrappers that can assume ownership of arbitrary objects, deleting
// them when the cursor is destroyed.

#ifndef SUPERSONIC_CURSOR_CORE_OWNERSHIP_TAKER_H_
#define SUPERSONIC_CURSOR_CORE_OWNERSHIP_TAKER_H_

#include "supersonic/utils/std_namespace.h"

#include "supersonic/utils/macros.h"

#include "supersonic/base/exception/exception_macros.h"
#include "supersonic/cursor/base/cursor.h"
#include "supersonic/cursor/base/operation.h"
#include "supersonic/cursor/proto/cursors.pb.h"

namespace supersonic {

template<typename Owned>
class OwnershipTaker : public Cursor {
 public:
  virtual ~OwnershipTaker() {}

  static unique_ptr<OwnershipTaker> Create(unique_ptr<Cursor> child, unique_ptr<Owned> owned) {
    return make_unique<OwnershipTaker>(std::move(child), std::move(owned));
  }

  virtual const TupleSchema& schema() const { return child_->schema(); }

  virtual ResultView Next(rowcount_t max_row_count) {
    return child_->Next(max_row_count);
  }

  virtual void Interrupt() { child_->Interrupt(); }

  virtual void AppendDebugDescription(string* target) const {
    child_->AppendDebugDescription(target);
  }

  virtual CursorId GetCursorId() const { return OWNERSHIP_TAKER; }

  OwnershipTaker(unique_ptr<Cursor> child, unique_ptr<Owned> owned)
      : owned_(std::move(owned)),
        child_(std::move(child))
  {}

private:
  // Defining owned_ field first, so it will outlive child_.
  unique_ptr<Owned> owned_;
  const unique_ptr<Cursor> child_;
  DISALLOW_COPY_AND_ASSIGN(OwnershipTaker);
};

namespace internal {

template<typename A, typename B>
auto new_linked_pair(unique_ptr<A> a, unique_ptr<B> b) {
  return make_unique<pair<linked_ptr<A>, linked_ptr<B>>>(
      make_linked_ptr(a.release()), make_linked_ptr(b.release()));
}

}  // namespace internal

// Take-ownership functions, for up to 6 arguments. (Can be extended further
// if needed). Return a cursor that, when itself destroyed, will destroy all
// the objects specified as arguments.

template<typename T>
unique_ptr<Cursor> TakeOwnership(unique_ptr<Cursor> child, unique_ptr<T> owned) {
  return OwnershipTaker<T>::Create(std::move(child), std::move(owned));
}

template<typename T, typename T2, typename... Args>
unique_ptr<Cursor> TakeOwnership(
    unique_ptr<Cursor> child,
    unique_ptr<T> a, unique_ptr<T2> b,
    Args&&... args) {
  return TakeOwnership(
      std::move(child),
      internal::new_linked_pair(std::move(a), std::move(b)),
      std::forward<Args>(args)...);
}

// Takes a dynamically-allocated operation and 'turns it' into a cursor;
// i.e., it creates a cursor from it, and make it the owner of the operation.
// The operation gets deleted when the cursor is deleted. You should not use
// the operation directly after having passed it to this function.
inline FailureOrOwned<Cursor> TurnIntoCursor(unique_ptr<Operation> operation) {
  FailureOrOwned<Cursor> result = operation->CreateCursor();
  PROPAGATE_ON_FAILURE(result);
  return Success(TakeOwnership(result.move(), std::move(operation)));
}

}  // namespace supersonic

#endif  // SUPERSONIC_CURSOR_CORE_OWNERSHIP_TAKER_H_
