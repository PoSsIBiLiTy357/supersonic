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
//
// Expression API.

#ifndef SUPERSONIC_EXPRESSION_BASE_EXPRESSION_H_
#define SUPERSONIC_EXPRESSION_BASE_EXPRESSION_H_

#include <set>

#include "supersonic/utils/std_namespace.h"

#include "supersonic/utils/macros.h"
#include "supersonic/base/exception/result.h"
#include "supersonic/base/infrastructure/bit_pointers.h"
#include "supersonic/base/infrastructure/block.h"
#include "supersonic/base/infrastructure/tuple_schema.h"
#include "supersonic/base/infrastructure/types.h"
#include "supersonic/utils/linked_ptr.h"

namespace supersonic {
class BufferAllocator;

// Result of expression evaluation. A thin wrapper over a view, exposed
// as a const reference.
typedef FailureOrReference<const View> EvaluationResult;

// 'Executable' expression. Types and properties (const, nullability etc.) are
// fully resolved. To evaluate it you need to pass a skip_vector to it.
class BoundExpression {
 public:
  virtual ~BoundExpression() {}

  // Returns the schema of the result.
  // Most expressions have a single-attribute result of some basic type.
  // Their result_schema will have just one attribute. In general though,
  // an expression may have an arbitrary (yet fixed) result schema.
  const TupleSchema& result_schema() const { return result_schema_; }

  // Runs the evaluation only for the rows for which the selection vector bit
  // is set to 1. Usually there is a single skip vector, but in general we
  // have a skip vector for each column in the result schema.
  virtual EvaluationResult DoEvaluate(const View& input,
                                      const BoolView& skip_vectors) = 0;

  // Returns the largest number of input rows this expression can be given
  // at input without failing due to buffer overflow.
  virtual rowcount_t row_capacity() const = 0;

  // Returns true if the expression is constant (that is, is of type Constant,
  // Null, or other such no-input, no-state, no-randomness type).
  virtual bool is_constant() const { return false; }

  // Returns a set of input schema attribute names that the expression depends
  // on. To be more formal: returns a minimal set of attributes names that had
  // to exist in the input tupleschema of the expression for successful
  // binding process.
  set<string> referred_attribute_names() const;

  // Adds to the set all names of input attributes that the expression
  // depends on. Does not remove the previous content of the set.
  virtual void CollectReferredAttributeNames(
      set<string>* referred_attribute_names) const = 0;

 protected:
  explicit BoundExpression(const TupleSchema& result_schema)
      : result_schema_(result_schema),
        view_(result_schema) {}

  View* my_view() { return &view_; }

 private:
  TupleSchema result_schema_;
  View view_;

  DISALLOW_COPY_AND_ASSIGN(BoundExpression);
};

// A tree of operations on which evaluation can be performed.
class BoundExpressionTree {
 public:
  // Note - a BoundExpressionTree is _not_ ready to use immediately after
  // creation! It will not be ready to use until Init is ran on it.
  explicit BoundExpressionTree(unique_ptr<BoundExpression> root,
                               BufferAllocator* allocator)
      : root_(std::move(root)),
        skip_vector_storage_(root_->result_schema().attribute_count(),
                             allocator) {}

  // Prepares the tree for usage, allocating the necessary memory.
  FailureOrVoid Init(BufferAllocator* allocator, rowcount_t max_row_count);

  const TupleSchema& result_schema() const { return root_->result_schema(); }

  // Causes the expression tree to be evaluated on the specified input view.
  // If successful, an EvaluationResult is returned, encapsulating a reference
  // to a result view with the same number of rows as the input view.
  // If failed, the result contains an Exception.
  // We use an pre-allocated empty skip_vector.
  EvaluationResult Evaluate(const View& input);

  rowcount_t row_capacity() const;

  bool is_constant() const { return root_->is_constant(); }

  // Returns a set of input schema attribute names that the expression depends
  // on. To be more formal: returns a minimal set of attributes names that had
  // to exist in the input tupleschema of the expression for successful
  // binding process.
  set<string> referred_attribute_names() const {
    return root_->referred_attribute_names();
  };

 private:
  // The encapsulated BoundExpression.
  std::unique_ptr<BoundExpression> root_;
  // Pre-allocated skip vectors for evaluation (one for each output column).
  BoolBlock skip_vector_storage_;

  DISALLOW_COPY_AND_ASSIGN(BoundExpressionTree);
};

// Creates and initializes a BoundExpressionTree that wraps the given
// BoundExpression. Takes ownership of the expression.
FailureOrOwned<BoundExpressionTree>
    CreateBoundExpressionTree(unique_ptr<BoundExpression> expression,
                              BufferAllocator* allocator,
                              rowcount_t max_row_count);

// 'Symbolic' expression. The result type is not yet known.
class Expression {
 public:
  virtual ~Expression() {}

  // Binds the expression to the input schema. Resolves all runtime types.
  // Caller takes ownership of the returned BoundExpressionTree.
  // If the expression can't be bound to the input schema, should return
  // an exception, with result code corresponding to the 'schema error'
  // range (400-499).
  // Return a fully evaluatable BoundExpressionTree, by encapsulating the
  // results of a DoBind in an BoundExpressionTree.
  FailureOrOwned<BoundExpressionTree> Bind(const TupleSchema& input_schema,
                                           BufferAllocator* allocator,
                                           rowcount_t max_row_count) const;

  // The function that does the actual binding, except for the encapsulation
  // within the tree structure.
  virtual FailureOrOwned<BoundExpression> DoBind(
      const TupleSchema& input_schema,
      BufferAllocator* allocator,
      rowcount_t max_row_count) const = 0;

  // Builds name for the expression. Traverses all children.
  // If verbose then more information will be generated.
  virtual string ToString(bool verbose) const = 0;

 protected:
  Expression() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Expression);
};

// Support for expressions that take variable lists of parameters (e.g. Concat).

// A list of bound expressions.
class BoundExpressionList final : private vector<unique_ptr<BoundExpression>> {
  using Inner = unique_ptr<BoundExpression>;

 public:
  using vector<Inner>::size;
  using vector<Inner>::operator[];
  using vector<Inner>::begin;
  using vector<Inner>::end;

  BoundExpressionList() {}

  template<typename... Args>
  BoundExpressionList(Args&&... args) {
    add(std::forward<Args>(args)...);
  }

  BoundExpressionList* add(Inner expression) {
    emplace_back(std::move(expression));
    return this;
  }

  template<typename... Args>
  BoundExpressionList* add(Inner expression, Args&&... args) {
    emplace_back(std::move(expression));
    add(std::forward<Args>(args)...);
    return this;
  }

  BoundExpression* get(int pos) const { return this->operator[](pos).get(); }
  unique_ptr<BoundExpression> move(int pos) { return std::move(this->operator[](pos)); }

  // Formats as: expr1, expr2, ... .
  const string ToString(bool verbose) const;

  // Appends to the set all names of input attributes that the expression
  // depends on. Does not remove the previous content of the set.
  void CollectReferredAttributeNames(set<string>* referred_attribute_names)
      const;

 private:

  DISALLOW_COPY_AND_ASSIGN(BoundExpressionList);
};

// A list of symbolic expressions.
class ExpressionList {
 public:
  ExpressionList() {}

  template<typename... Args>
  ExpressionList(Args&&... args)
  { add(std::forward<Args>(args)...); }

  template<typename... Args>
  ExpressionList* add(unique_ptr<const Expression> e, Args&&... args) {
    add(std::move(e));
    add(std::forward<Args>(args)...);
    return this;
  }

  ExpressionList* add(unique_ptr<const Expression> e) {
    expressions_.emplace_back(std::move(e));
    return this;
  }

  int size() const { return expressions_.size(); }

  FailureOrOwned<BoundExpressionList> DoBind(const TupleSchema& input_schema,
                                             BufferAllocator* allocator,
                                             rowcount_t max_row_count) const;
  // Formats as: expr1, expr2, ... .
  const string ToString(bool verbose) const;

 private:
  vector<unique_ptr<const Expression>> expressions_;
  DISALLOW_COPY_AND_ASSIGN(ExpressionList);
};

}  // namespace supersonic

#endif  // SUPERSONIC_EXPRESSION_BASE_EXPRESSION_H_
