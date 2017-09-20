// Copyright 2011 Google Inc. All Rights Reserved.
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

#include "supersonic/expression/core/stateful_expressions.h"

#include "supersonic/expression/core/stateful_bound_expressions.h"
#include "supersonic/expression/infrastructure/basic_expressions.h"

namespace supersonic {

unique_ptr<const Expression> Changed(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(
      std::move(argument), &BoundChanged, "CHANGED($0)");
}

unique_ptr<const Expression> RunningSum(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(
      std::move(argument), &BoundRunningSum, "RUNNING_SUM($0)");
}

unique_ptr<const Expression> Smudge(unique_ptr<const Expression> argument) {
  return CreateExpressionForExistingBoundFactory(
      std::move(argument), &BoundSmudge, "SMUDGE($0)");
}

unique_ptr<const Expression> RunningMinWithFlush(
    unique_ptr<const Expression> flush, unique_ptr<const Expression> input) {
  return CreateExpressionForExistingBoundFactory(
      std::move(flush), std::move(input), &BoundRunningMinWithFlush,
      "RUNNING_MIN_WITH_FLUSH($0, $1)");
}

unique_ptr<const Expression> SmudgeIf(unique_ptr<const Expression> argument,
                                      unique_ptr<const Expression> condition) {
  return CreateExpressionForExistingBoundFactory(
      std::move(argument), std::move(condition), &BoundSmudgeIf,
      "SMUDGE_IF($0, $1)");
}

}  // namespace supersonic
