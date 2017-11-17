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
// Author:  onufry@google.com (Jakub Onufry Wojtaszczyk)

#include "supersonic/expression/infrastructure/terminal_expressions.h"

#include <memory>
#include <string>
namespace supersonic {using std::string; }

#include <glog/logging.h>
#include "supersonic/utils/logging-inl.h"
#include "supersonic/utils/macros.h"
#include "supersonic/base/exception/result.h"
#include "supersonic/expression/base/expression.h"
#include "supersonic/expression/infrastructure/elementary_const_expressions.h"
#include "supersonic/expression/infrastructure/terminal_bound_expressions.h"
#include "supersonic/utils/strings/join.h"
#include "supersonic/utils/random.h"

namespace supersonic {

// ----------------------------------------------------------------------------
// Terminal Expressions.

class BufferAllocator;
class TupleSchema;

namespace {

class NullExpression : public Expression {
 public:
  explicit NullExpression(DataType type) : type_(type) {}
  virtual FailureOrOwned<BoundExpression> DoBind(
      const TupleSchema& input_schema,
      BufferAllocator* allocator,
      rowcount_t max_row_count) const {
    return BoundNull(type_, allocator, max_row_count);
  }

  string ToString(bool verbose) const {
    if (verbose)
      return StrCat("<", DataType_Name(type_), ">NULL");
    else
      return "NULL";
  }

 private:
  const DataType type_;
  friend class BuildExpressionFromProtoTest;
  DISALLOW_COPY_AND_ASSIGN(NullExpression);
};

class SequenceExpression : public Expression {
 public:
  SequenceExpression() {}
  virtual FailureOrOwned<BoundExpression> DoBind(
      const TupleSchema& input_schema,
      BufferAllocator* allocator,
      rowcount_t max_row_count) const {
    return BoundSequence(allocator, max_row_count);
  }

  string ToString(bool verbose) const { return "SEQUENCE()"; }

 private:
  DISALLOW_COPY_AND_ASSIGN(SequenceExpression);
};

class RandInt32Expression : public Expression {
 public:
  explicit RandInt32Expression(unique_ptr<RandomBase> random_generator)
    : random_generator_(std::move(random_generator)) {}
  virtual FailureOrOwned<BoundExpression> DoBind(
      const TupleSchema& input_schema,
      BufferAllocator* allocator,
      rowcount_t max_row_count) const {
    auto generator_clone = random_generator_->Clone();
    CHECK_NOTNULL(generator_clone.get());
    return BoundRandInt32(std::move(generator_clone), allocator, max_row_count);
  }

  string ToString(bool verbose) const { return "RANDINT32()"; }

 private:
  std::unique_ptr<RandomBase> random_generator_;
  DISALLOW_COPY_AND_ASSIGN(RandInt32Expression);
};

}  // namespace

unique_ptr<const Expression> Null(DataType type) {
  return make_unique<NullExpression>(type);
}

unique_ptr<const Expression> Sequence() {
  return make_unique<SequenceExpression>();
}

unique_ptr<const Expression> RandInt32(
    unique_ptr<RandomBase> random_generator) {
  return make_unique<RandInt32Expression>(std::move(random_generator));
}

unique_ptr<const Expression> RandInt32() {
  return RandInt32(make_unique<MTRandom>());
}

unique_ptr<const Expression> ConstInt32(const int32_t& value) {
  return make_unique<ConstExpression<INT32>>(value);
}

unique_ptr<const Expression> ConstInt64(const int64_t& value) {
  return make_unique<ConstExpression<INT64>>(value);
}

unique_ptr<const Expression> ConstUint32(const uint32_t& value) {
  return make_unique<ConstExpression<UINT32>>(value);
}

unique_ptr<const Expression> ConstUint64(const uint64_t& value) {
  return make_unique<ConstExpression<UINT64>>(value);
}

unique_ptr<const Expression> ConstFloat(const float& value) {
  return make_unique<ConstExpression<FLOAT>>(value);
}

unique_ptr<const Expression> ConstDouble(const double& value) {
  return make_unique<ConstExpression<DOUBLE>>(value);
}

unique_ptr<const Expression> ConstBool(const bool& value) {
  return make_unique<ConstExpression<BOOL>>(value);
}

unique_ptr<const Expression> ConstDate(const int32_t& value) {
  return make_unique<ConstExpression<DATE>>(value);
}

unique_ptr<const Expression> ConstDateTime(const int64_t& value) {
  return make_unique<ConstExpression<DATETIME>>(value);
}

unique_ptr<const Expression> ConstString(const StringPiece& value) {
  return make_unique<ConstExpression<STRING>>(value);
}

unique_ptr<const Expression> ConstBinary(const StringPiece& value) {
  return make_unique<ConstExpression<BINARY>>(value);
}

unique_ptr<const Expression> ConstDataType(const DataType& value) {
  return make_unique<ConstExpression<DATA_TYPE>>(value);
}

}  // namespace supersonic
