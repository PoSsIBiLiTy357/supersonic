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
// Limit cursor & operation.

#ifndef SUPERSONIC_CURSOR_CORE_LIMIT_H_
#define SUPERSONIC_CURSOR_CORE_LIMIT_H_

#include "supersonic/utils/std_namespace.h"
#include "supersonic/base/infrastructure/types.h"

namespace supersonic {

// Creates a LIMIT operation, with the specified limit and offset.
class Cursor;
class Operation;

unique_ptr<Operation> Limit(rowcount_t offset, rowcount_t limit, unique_ptr<Operation> child);

// Bound version of the above.
unique_ptr<Cursor> BoundLimit(rowcount_t offset, rowcount_t limit, unique_ptr<Cursor> child);

}  // namespace supersonic

#endif  // SUPERSONIC_CURSOR_CORE_LIMIT_H_
