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

#ifndef SUPERSONIC_CURSOR_CORE_ROWID_MERGE_JOIN_H_
#define SUPERSONIC_CURSOR_CORE_ROWID_MERGE_JOIN_H_

#include "supersonic/utils/stl_util.h"

namespace supersonic {

class BoundMultiSourceProjector;
class BoundSingleSourceProjector;
class Cursor;
class MultiSourceProjector;
class SingleSourceProjector;
class BufferAllocator;
class Operation;

// Creates an inner join on (left column key == right row-id).
// left_key_selector indicates the column in the left-hand side input to be
// used as the key. It must be int64 NOT NULL, and it must be sorted in the
// ascending order (or it crashes). It is matched against a right row-id
// (absolute row index), counted from zero. result_projector indicates columns
// to be included in the output. It must be a two-source projector (left, then
// right). Takes ownership of all projectors.
// Enforces referential integrity: if the left key refers to a non-existing
// row ID in the right cursor, the cursor fails with an error.
unique_ptr<Operation> RowidMergeJoin(
    unique_ptr<const SingleSourceProjector> left_key_selector,
    unique_ptr<const MultiSourceProjector> result_projector,
    unique_ptr<Operation> left,
    unique_ptr<Operation> right);

// Bound version of the above.
unique_ptr<Cursor> BoundRowidMergeJoin(
    unique_ptr<const BoundSingleSourceProjector> left_key_selector,
    unique_ptr<const BoundMultiSourceProjector> result_projector,
    unique_ptr<Cursor> left,
    unique_ptr<Cursor> right,
    BufferAllocator* allocator);

}  // namespace supersonic

#endif  // SUPERSONIC_CURSOR_CORE_ROWID_MERGE_JOIN_H_
