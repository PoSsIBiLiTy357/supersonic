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

#include "supersonic/expression/core/projecting_expressions.h"

#include "supersonic/utils/std_namespace.h"
#include "supersonic/base/exception/exception.h"
#include "supersonic/base/exception/exception_macros.h"
#include "supersonic/expression/base/expression.h"
#include "supersonic/expression/core/projecting_bound_expressions.h"

namespace supersonic {
class BufferAllocator;
class TupleSchema;

namespace {

class InputAttributeProjectionExpression : public Expression {
 public:
  explicit InputAttributeProjectionExpression(
      unique_ptr<const SingleSourceProjector> projector)
      : projector_(std::move(projector)) {}
  virtual FailureOrOwned<BoundExpression> DoBind(
      const TupleSchema& input_schema,
      BufferAllocator* allocator,
      rowcount_t max_row_count) const {
    return BoundInputAttributeProjection(input_schema, *projector_);
  }

  virtual string ToString(bool verbose) const {
    return projector_->ToString(verbose);
  }

 private:
  unique_ptr<const SingleSourceProjector> projector_;
  DISALLOW_COPY_AND_ASSIGN(InputAttributeProjectionExpression);
};

// Helper, used by ProjectionExpression and CompoundExpression. Does not take
// the ownership of arguments or the projector.
FailureOrOwned<BoundExpression> CreateBoundProjection(
    const TupleSchema& input_schema,
    BufferAllocator* allocator,
    rowcount_t max_row_count,
    const ExpressionList* arguments,
    const MultiSourceProjector* projector) {
  FailureOrOwned<BoundExpressionList> bound_arguments(
      arguments->DoBind(input_schema, allocator, max_row_count));
  PROPAGATE_ON_FAILURE(bound_arguments);
  vector<TupleSchema> schemata;
  for (auto const& expr: *bound_arguments) {
    schemata.emplace_back(expr->result_schema());
  }
  FailureOrOwned<const BoundMultiSourceProjector> bound_projector(
      projector->Bind(schemata));
  PROPAGATE_ON_FAILURE(bound_projector);
  return BoundProjection(bound_projector.move(),
                         bound_arguments.move());
}

// Similar to CompoundExpression, but CompoundExpression supports
// CompoundMultiSourceProjector only, while having a convenient build API.
class ProjectionExpression : public Expression {
 public:
  explicit ProjectionExpression(
      unique_ptr<const ExpressionList> arguments,
      unique_ptr<const MultiSourceProjector> projector)
      : arguments_(std::move(arguments)),
        projector_(std::move(projector)) {}
  virtual FailureOrOwned<BoundExpression> DoBind(
      const TupleSchema& input_schema,
      BufferAllocator* allocator,
      rowcount_t max_row_count) const {
    return CreateBoundProjection(input_schema, allocator, max_row_count,
                                 arguments_.get(), projector_.get());
  }

  // Formatted string contains arguments_, verbose version appends also
  // projector.
  virtual string ToString(bool verbose) const {
    if (verbose) {
      return StrCat(
          projector_->ToString(verbose), ": ", arguments_->ToString(verbose));
    }
    return arguments_->ToString(verbose);
  }

 private:
  unique_ptr<const ExpressionList> arguments_;
  unique_ptr<const MultiSourceProjector> projector_;
  DISALLOW_COPY_AND_ASSIGN(ProjectionExpression);
};

}  // namespace

// ------------------------ Expression instantiations --------------------------

unique_ptr<const Expression> InputAttributeProjection(
    unique_ptr<const SingleSourceProjector> projector) {
  return make_unique<InputAttributeProjectionExpression>(std::move(projector));
}

unique_ptr<const Expression> Projection(
    unique_ptr<const ExpressionList> inputs,
    unique_ptr<const MultiSourceProjector> projector) {
  return make_unique<ProjectionExpression>(std::move(inputs),
                                           std::move(projector));
}

// NOTE(onufry): This implementation runs through CompoundExpression, which has
// quite a lot of logic inside that is definitely not necessary for the Alias
// (mostly tied to short circuit, which in the case of Alias is trivial). If the
// performance ever becomes a problem here, we should write a specialized
// expresion for this.
unique_ptr<const Expression> Alias(const string& new_name,
                        unique_ptr<const Expression> argument) {
  auto out = make_unique<CompoundExpression>();
  out->AddAs(new_name, std::move(argument));
  return out;
}

// ------------------------ Implementation details -----------------------------

CompoundExpression* CompoundExpression::Add(unique_ptr<const Expression> argument) {
  size_t argument_index = arguments_->size();
  arguments_->add(std::move(argument));
  projector_->add(argument_index, ProjectAllAttributes());
  return this;
}

CompoundExpression* CompoundExpression::AddAs(const StringPiece& alias,
                                              unique_ptr<const Expression> argument) {
  return AddAsMulti(vector<string>(1, alias.ToString()), std::move(argument));
}

CompoundExpression* CompoundExpression::AddAsMulti(
    const vector<string>& aliases,
    unique_ptr<const Expression> argument) {
  size_t argument_index = arguments_->size();
  arguments_->add(std::move(argument));
  projector_->add(argument_index,
                  ProjectRename(aliases, ProjectAllAttributes()));
  return this;
}

FailureOrOwned<BoundExpression> CompoundExpression::DoBind(
    const TupleSchema& input_schema,
    BufferAllocator* allocator,
    rowcount_t max_row_count) const {
  return CreateBoundProjection(input_schema, allocator, max_row_count,
                               arguments_.get(), projector_.get());
}

}  // namespace supersonic
