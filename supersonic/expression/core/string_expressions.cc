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

#include "supersonic/expression/core/string_expressions.h"

#include <memory>
#include <string>

#include "supersonic/utils/std_namespace.h"
#include "supersonic/base/exception/exception.h"
#include "supersonic/base/exception/exception_macros.h"
#include "supersonic/base/exception/result.h"
#include "supersonic/base/infrastructure/types.h"
#include "supersonic/expression/base/expression.h"
#include "supersonic/expression/core/string_bound_expressions.h"
#include "supersonic/expression/core/string_evaluators.h"  // IWYU pragma: keep
#include "supersonic/expression/infrastructure/basic_expressions.h"
#include "supersonic/utils/strings/strcat.h"

namespace supersonic {

class BufferAllocator;
class TupleSchema;

namespace {

// The concatenation expression, extending expression. It doesn't fit into the
// general scheme of abstract_expressions.h as it has an arbitrary number of
// arguments.
class ConcatExpression : public Expression {
 public:
  explicit ConcatExpression(unique_ptr<const ExpressionList> list)
      : args_(std::move(list)) {}

 private:
  virtual FailureOrOwned<BoundExpression> DoBind(
      const TupleSchema& input_schema,
      BufferAllocator* allocator,
      rowcount_t max_row_count) const {
    FailureOrOwned<BoundExpressionList> args = args_->DoBind(
        input_schema, allocator, max_row_count);
    PROPAGATE_ON_FAILURE(args);
    return BoundConcat(args.move(), allocator, max_row_count);
  }

  virtual string ToString(bool verbose) const {
    return StrCat("CONCAT(", args_.get()->ToString(verbose), ")");
  }

  const unique_ptr<const ExpressionList> args_;
};

}  // namespace

unique_ptr<const Expression> Concat(unique_ptr<const ExpressionList> args) {
  return make_unique<ConcatExpression>(std::move(args));
}

unique_ptr<const Expression> Length(unique_ptr<const Expression> str) {
  return CreateExpressionForExistingBoundFactory(std::move(str), &BoundLength,
                                                 "LENGTH($0)");
}

unique_ptr<const Expression> Ltrim(unique_ptr<const Expression> str) {
  return CreateExpressionForExistingBoundFactory(std::move(str), &BoundLtrim,
                                                 "LTRIM($0)");
}

unique_ptr<const Expression> Rtrim(unique_ptr<const Expression> str) {
  return CreateExpressionForExistingBoundFactory(std::move(str), &BoundRtrim,
                                                 "RTRIM($0)");
}

unique_ptr<const Expression>
StringContains(unique_ptr<const Expression> haystack,
               unique_ptr<const Expression> needle) {
  return CreateExpressionForExistingBoundFactory(
      std::move(haystack), std::move(needle), &BoundContains,
      "CONTAINS($0, $1)");
}

unique_ptr<const Expression>
StringContainsCI(unique_ptr<const Expression> haystack,
                 unique_ptr<const Expression> needle) {
  return CreateExpressionForExistingBoundFactory(
      std::move(haystack), std::move(needle), &BoundContainsCI,
      "CONTAINS_CI($0, $1)");
}

unique_ptr<const Expression> StringOffset(unique_ptr<const Expression> haystack,
                                          unique_ptr<const Expression> needle) {
  return CreateExpressionForExistingBoundFactory(
      std::move(haystack), std::move(needle), &BoundStringOffset,
      "STRING_OFFSET($0, $1)");
}

unique_ptr<const Expression>
StringReplace(unique_ptr<const Expression> haystack,
              unique_ptr<const Expression> needle,
              unique_ptr<const Expression> substitute) {
  return CreateExpressionForExistingBoundFactory(
      std::move(haystack), std::move(needle), std::move(substitute),
      &BoundStringReplace, "STRING_REPLACE($0, $1, $2)");
}

unique_ptr<const Expression> Substring(unique_ptr<const Expression> str,
                                       unique_ptr<const Expression> pos,
                                       unique_ptr<const Expression> length) {
  return CreateExpressionForExistingBoundFactory(
      std::move(str), std::move(pos), std::move(length), &BoundSubstring,
      "SUBSTRING($0, $1, $2)");
}

unique_ptr<const Expression> ToLower(unique_ptr<const Expression> str) {
  return CreateExpressionForExistingBoundFactory(std::move(str), &BoundToLower,
                                                 "TO_LOWER($0)");
}

unique_ptr<const Expression> ToString(unique_ptr<const Expression> expr) {
  return CreateExpressionForExistingBoundFactory(
      std::move(expr), &BoundToString, "TO_STRING($0)");
}

unique_ptr<const Expression> ToUpper(unique_ptr<const Expression> str) {
  return CreateExpressionForExistingBoundFactory(std::move(str), &BoundToUpper,
                                                 "TO_UPPER($0)");
}

unique_ptr<const Expression>
TrailingSubstring(unique_ptr<const Expression> str,
                  unique_ptr<const Expression> pos) {
  return CreateExpressionForExistingBoundFactory(std::move(str), std::move(pos),
                                                 &BoundTrailingSubstring,
                                                 "SUBSTRING($0, $1)");
}

unique_ptr<const Expression> Trim(unique_ptr<const Expression> str) {
  return CreateExpressionForExistingBoundFactory(std::move(str), &BoundTrim,
                                                 "TRIM($0)");
}

}  // namespace supersonic
