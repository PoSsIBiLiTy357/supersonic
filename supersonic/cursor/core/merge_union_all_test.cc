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

#include "supersonic/cursor/core/merge_union_all.h"

#include <memory>
#include <vector>
using std::vector;

#include "supersonic/base/infrastructure/types.h"
#include "supersonic/cursor/base/cursor.h"
#include "supersonic/cursor/base/cursor_transformer.h"
#include "supersonic/cursor/core/spy.h"
#include "supersonic/cursor/infrastructure/ordering.h"
#include "supersonic/proto/supersonic.pb.h"
#include "supersonic/testing/block_builder.h"
#include "supersonic/testing/comparators.h"
#include "supersonic/testing/operation_testing.h"
#include "gtest/gtest.h"
#include "supersonic/utils/container_literal.h"

namespace supersonic {

class MergeUnionAllTest : public testing::TestWithParam<int> {
 public:
  void SetUp() {
    a1_.AddRow("a", 1);

    a2_.AddRow("a", 2);

    a3_.AddRow("a", 3);

    b2_.AddRow("b", 2);

    c3_.AddRow("c", 3);

    n1_.AddRow(__, 1);

    a1a1a2_.AddRow("a", 1)
           .AddRow("a", 1)
           .AddRow("a", 2);

    a1a2a3_.AddRow("a", 1)
           .AddRow("a", 2)
           .AddRow("a", 3);

    b2b3b4_.AddRow("b", 2)
           .AddRow("b", 3)
           .AddRow("b", 4);

    c3c4c5_.AddRow("c", 3)
           .AddRow("c", 4)
           .AddRow("c", 5);

    n1n1a1_.AddRow(__, 1)
           .AddRow(__, 1)
           .AddRow("a", 1);

    a1a2b1b2_.AddRow("a", 1)
             .AddRow("a", 2)
             .AddRow("b", 1)
             .AddRow("b", 2);

    a1a3b2b2_.AddRow("a", 1)
             .AddRow("a", 3)
             .AddRow("b", 2)
             .AddRow("b", 2);

    a2b3_.AddRow("a", 2)
         .AddRow("b", 3);

    a1a2b1b2_a1a3b2b2_output_.AddRow("a", 1)
                             .AddRow("a", 1)
                             .AddRow("a", 2)
                             .AddRow("a", 3)
                             .AddRow("b", 1)
                             .AddRow("b", 2)
                             .AddRow("b", 2)
                             .AddRow("b", 2);
  }

 protected:
  TestDataBuilder<STRING, INT64>
    a1_, a2_, a3_, b2_, c3_, n1_, empty_,
    a1a1a2_, a1a2a3_, b2b3b4_, c3c4c5_, n1n1a1_,
    a1a2b1b2_, a1a3b2b2_, a2b3_;

  TestDataBuilder<STRING, INT64> a1a2b1b2_a1a3b2b2_output_;
};

template<typename T>
void fill_uptr_vector(vector<unique_ptr<T>>& out) {

}

template<typename B, typename T, typename... Args>
void fill_uptr_vector(vector<unique_ptr<B>>& out, unique_ptr<T> arg, Args... args) {
  out.emplace_back(std::move(arg));
  fill_uptr_vector(out, std::forward<Args>(args)...);
}

template<typename T, typename... Args>
vector<unique_ptr<T>> uptr_vector(Args&&... args) {
  vector<unique_ptr<T>> out;
  fill_uptr_vector<T>(out, std::forward<Args>(args)...);
  return out;
}

TEST_F(MergeUnionAllTest, NoSourcesGivesEmptyResults) {
  OperationTest test;
  test.SetExpectedResult(TestDataBuilder<>().Build());
  auto order = make_unique<SortOrder>();
  order->OrderByNamedAttribute("col0", ASCENDING)
       ->OrderByNamedAttribute("col1", ASCENDING);
  test.Execute(MergeUnionAll(
      std::move(order),
      uptr_vector<Operation>()));
}

TEST_F(MergeUnionAllTest, empty_a1) {
  OperationTest test;
  test.SetExpectedResult(a1_.Build());
  vector<unique_ptr<Operation>> inputs;
  inputs.emplace_back(empty_.Build());
  inputs.emplace_back(a1_.Build());
  auto order = make_unique<SortOrder>();
  order->OrderByNamedAttribute("col0", ASCENDING)
       ->OrderByNamedAttribute("col1", ASCENDING);
  test.Execute(MergeUnionAll(
      std::move(order),
      std::move(inputs)));
}

TEST_F(MergeUnionAllTest, a1_empty) {
  OperationTest test;
  test.SetExpectedResult(a1_.Build());
  auto order = make_unique<SortOrder>();
  order->OrderByNamedAttribute("col0", ASCENDING)
       ->OrderByNamedAttribute("col1", ASCENDING);
  test.Execute(MergeUnionAll(
      std::move(order),
      uptr_vector<Operation>(a1_.Build(), empty_.Build())));
}

TEST_F(MergeUnionAllTest, a1a2b1b2_a1a3b2b2) {
  OperationTest test;
  test.AddInput(a1a2b1b2_.Build());
  test.AddInput(a1a3b2b2_.Build());
  test.SetExpectedResult(a1a2b1b2_a1a3b2b2_output_.Build());
  auto order = make_unique<SortOrder>();
  order->OrderByNamedAttribute("col0", ASCENDING)
       ->OrderByNamedAttribute("col1", ASCENDING);
  test.Execute(MergeUnionAll(
      std::move(order),
      uptr_vector<Operation>(test.input_at(0) , test.input_at(1))));
}

TEST_F(MergeUnionAllTest, a1a2b1b2_a1a3b2b2WithSpyTransform) {
  auto input1 = a1a2b1b2_.BuildCursor();
  auto input2 = a1a3b2b2_.BuildCursor();

  std::unique_ptr<Cursor> expected_result(
      a1a2b1b2_a1a3b2b2_output_.BuildCursor());

  auto sort_order = make_unique<SortOrder>();
  sort_order->OrderByNamedAttribute("col0", ASCENDING);
  sort_order->OrderByNamedAttribute("col1", ASCENDING);
  auto bound_sort_order = SucceedOrDie(sort_order->Bind(input1->schema()));

  std::unique_ptr<Cursor> merge(SucceedOrDie(
      BoundMergeUnionAll(std::move(bound_sort_order),
                         uptr_vector<Cursor>(std::move(input1), std::move(input2)),
                         HeapBufferAllocator::Get())));

  std::unique_ptr<CursorTransformerWithSimpleHistory> spy_transformer(
      PrintingSpyTransformer());
  merge->ApplyToChildren(spy_transformer.get());
  merge = spy_transformer->Transform(std::move(merge));

  EXPECT_CURSORS_EQUAL(std::move(expected_result), std::move(merge));
}

TEST_F(MergeUnionAllTest, RegressionTestForRowComparisonBug) {
  OperationTest test;
  test.SetExpectedResult(TestDataBuilder<STRING, INT64>()
                         .AddRow("a", 1)
                         .AddRow("a", 1)
                         .AddRow("a", 2)
                         .AddRow("a", 2)
                         .AddRow("b", 1)
                         .AddRow("b", 1)
                         .Build());
  TestDataBuilder<STRING, INT64> double_input;
  double_input.AddRow("a", 1)
              .AddRow("a", 2)
              .AddRow("b", 1);
  auto sort_order = make_unique<SortOrder>();
  sort_order->OrderByNamedAttribute("col0", ASCENDING);
  sort_order->OrderByNamedAttribute("col1", ASCENDING);
  test.Execute(MergeUnionAll(
      std::move(sort_order),
      uptr_vector<Operation>(double_input.Build(), double_input.Build())));
}


TEST_F(MergeUnionAllTest, c3_b2_a3_a2_a1) {
  OperationTest test;
  test.SetExpectedResult(TestDataBuilder<STRING, INT64>()
                         .AddRow("a", 1)
                         .AddRow("a", 2)
                         .AddRow("a", 3)
                         .AddRow("b", 2)
                         .AddRow("c", 3)
                         .Build());
  auto sort_order = make_unique<SortOrder>();
  sort_order->OrderByNamedAttribute("col0", ASCENDING);
  sort_order->OrderByNamedAttribute("col1", ASCENDING);
  test.Execute(MergeUnionAll(
      std::move(sort_order),
      uptr_vector<Operation>(c3_.Build(), b2_.Build(), a3_.Build(),
                             a2_.Build(), a1_.Build())));
}

TEST_F(MergeUnionAllTest, b2_c3_a3_a2_a1) {
  OperationTest test;
  test.SetExpectedResult(TestDataBuilder<STRING, INT64>()
                         .AddRow("a", 1)
                         .AddRow("a", 2)
                         .AddRow("a", 3)
                         .AddRow("b", 2)
                         .AddRow("c", 3)
                         .Build());
  auto sort_order = make_unique<SortOrder>();
  sort_order->OrderByNamedAttribute("col0", ASCENDING);
  sort_order->OrderByNamedAttribute("col1", ASCENDING);
  test.Execute(MergeUnionAll(
      std::move(sort_order),
      uptr_vector<Operation>(b2_.Build(), c3_.Build(), a3_.Build(),
                             a2_.Build(), a1_.Build())));
}

TEST_F(MergeUnionAllTest, a2_a1) {
  OperationTest test;
  test.SetExpectedResult(TestDataBuilder<STRING, INT64>()
                         .AddRow("a", 1)
                         .AddRow("a", 2)
                         .Build());
  auto sort_order = make_unique<SortOrder>();
  sort_order->OrderByNamedAttribute("col0", ASCENDING);
  sort_order->OrderByNamedAttribute("col1", ASCENDING);
  test.Execute(MergeUnionAll(
      std::move(sort_order),
      uptr_vector<Operation>(a2_.Build(), a1_.Build())));
}

TEST_F(MergeUnionAllTest, a1) {
  OperationTest test;
  test.SetExpectedResult(a1_.Build());
  auto sort_order = make_unique<SortOrder>();
  sort_order->OrderByNamedAttribute("col0", ASCENDING);
  test.Execute(MergeUnionAll(
      std::move(sort_order),
      uptr_vector<Operation>(a1_.Build())));
}

TEST_F(MergeUnionAllTest, a1a1a2) {
  OperationTest test;
  test.SetExpectedResult(a1a1a2_.Build());
  auto sort_order = make_unique<SortOrder>();
  sort_order->OrderByAttributeAt(1, ASCENDING);
  test.Execute(MergeUnionAll(
      std::move(sort_order),
      uptr_vector<Operation>(a1a1a2_.Build())));
}

TEST_F(MergeUnionAllTest,
       a1_b2_c3) {
  OperationTest test;
  test.SetExpectedResult(TestDataBuilder<STRING, INT64>()
      .AddRow("a", 1)
      .AddRow("b", 2)
      .AddRow("c", 3)
      .Build());
  auto sort_order = make_unique<SortOrder>();
  sort_order->OrderByAttributeAt(1, ASCENDING);
  test.Execute(MergeUnionAll(
      std::move(sort_order),
      uptr_vector<Operation>(a1_.Build(), b2_.Build(), c3_.Build())));
}

TEST_F(MergeUnionAllTest, a1a2a3_b2b3b4_c3c4c5) {
  OperationTest test;
  test.SetExpectedResult(TestDataBuilder<STRING, INT64>()
      .AddRow("a", 1)
      .AddRow("a", 2)
      .AddRow("b", 2)
      .AddRow("a", 3)
      .AddRow("b", 3)
      .AddRow("c", 3)
      .AddRow("b", 4)
      .AddRow("c", 4)
      .AddRow("c", 5)
      .Build());
  auto sort_order = make_unique<SortOrder>();
  sort_order
      ->OrderByAttributeAt(1, ASCENDING)
      ->OrderByAttributeAt(0, ASCENDING);
  test.Execute(MergeUnionAll(
      std::move(sort_order),
      uptr_vector<Operation>(a1a2a3_.Build(), b2b3b4_.Build(), c3c4c5_.Build())));
}

// Checks if MergeUnionAll is deterministic by having input sources return
// WaitingOnBarrier at different points and checking if the resulting rows are
// still returned in the same order.
TEST_F(MergeUnionAllTest, CheckDeterminism) {
  static const int kNumInputs = 4;
  TestDataBuilder<STRING, INT64>* inputs[4] = { &a1a1a2_, &a1a2a3_,
                                                &a1a1a2_, &a1a2a3_ };

  OperationTest test;
  auto sort_order = make_unique<SortOrder>();
  sort_order->OrderByAttributeAt(0, ASCENDING);
  test.SetExpectedResult(MergeUnionAll(
      std::move(sort_order),
      uptr_vector<Operation>(inputs[0]->Build(), inputs[1]->Build(),
                             inputs[2]->Build(), inputs[3]->Build())));

  for (int i = 0; i < kNumInputs; ++i) {
    test.AddInput(inputs[i]->Build());
  }

  // OperationTest::Execute repedeately executes the operation, each time:
  //   - changing the number of rows returned by inputs at once
  //   - changing the number of rows read on output at once
  //   - injects WaitOnBarrier at various points in time.
  sort_order = make_unique<SortOrder>();
  sort_order->OrderByAttributeAt(0, ASCENDING);
  test.Execute(MergeUnionAll(
      std::move(sort_order),
      uptr_vector<Operation>(test.input_at(0), test.input_at(1),
                           test.input_at(2), test.input_at(3))));
}

TEST_F(MergeUnionAllTest, a2_a1_a3) {
  OperationTest test;
  test.SetExpectedResult(a1a2a3_.Build());
  auto sort_order = make_unique<SortOrder>();
  sort_order
      ->OrderByAttributeAt(0, ASCENDING)
      ->OrderByAttributeAt(1, ASCENDING);
  test.Execute(MergeUnionAll(
      std::move(sort_order),
      uptr_vector<Operation>(a1_.Build(), a2_.Build(), a3_.Build())));
}

TEST_F(MergeUnionAllTest, a1a2b1b2_a1a3b2b2_a2b3) {
  OperationTest test;
  test.SetExpectedResult(TestDataBuilder<STRING, INT64>()
      .AddRow("a", 1)
      .AddRow("a", 1)
      .AddRow("a", 2)
      .AddRow("a", 2)
      .AddRow("a", 3)
      .AddRow("b", 1)
      .AddRow("b", 2)
      .AddRow("b", 2)
      .AddRow("b", 2)
      .AddRow("b", 3)
      .Build());
  auto sort_order = make_unique<SortOrder>();
  sort_order
      ->OrderByAttributeAt(0, ASCENDING)
      ->OrderByAttributeAt(1, ASCENDING);
  test.Execute(MergeUnionAll(
      std::move(sort_order),
      uptr_vector<Operation>(a1a2b1b2_.Build(), a1a3b2b2_.Build(), a2b3_.Build())));
}

TEST_F(MergeUnionAllTest, ManyInputs) {
  OperationTest test;
  const int kNumInputs = 100;
  const rowcount_t kNumRowsPerInput = 5;
  vector<unique_ptr<Operation>> inputs;
  for (int i = 1; i <= kNumInputs; i++) {
    TestDataBuilder<INT64, INT64> builder;
    for (rowid_t j = kNumRowsPerInput; j > 0; j--)
      builder.AddRow(i, j);
    inputs.emplace_back(builder.Build());
  }
  TestDataBuilder<INT64, INT64> expected_output;
  for (rowid_t j = kNumRowsPerInput; j > 0; j--) {
    for (int i = kNumInputs; i > 0; i--) {
      expected_output.AddRow(i, j);
    }
  }
  auto sort_order = make_unique<SortOrder>();
  sort_order
      ->OrderByAttributeAt(1, DESCENDING)
      ->OrderByAttributeAt(0, DESCENDING);
  test.SetExpectedResult(expected_output.Build());
  test.Execute(MergeUnionAll(std::move(sort_order), std::move(inputs)));
}

TEST_F(MergeUnionAllTest, a1a1a1a1a1a1a1b1_b2) {
  OperationTest test;
  TestDataBuilder<STRING, INT64> builder;
  builder.AddRow("a", 1)
         .AddRow("a", 1)
         .AddRow("a", 1)
         .AddRow("a", 1)
         .AddRow("a", 1)
         .AddRow("a", 1)
         .AddRow("a", 1)
         .AddRow("b", 1);
  // Create first input, add ("b", 2) and then use the same builder to create
  // expected result.
  auto input1 = builder.Build();

  vector<unique_ptr<Operation>> inputs;
  inputs.emplace_back(std::move(input1));
  inputs.emplace_back(b2_.Build());

  auto sort_order = make_unique<SortOrder>();
  sort_order
      ->OrderByAttributeAt(0, ASCENDING)
      ->OrderByAttributeAt(1, ASCENDING);

  builder.AddRow("b", 2);
  test.SetExpectedResult(builder.Build());
  test.Execute(MergeUnionAll(std::move(sort_order), std::move(inputs)));
}

TEST_F(MergeUnionAllTest, a1_n1_n1) {
  OperationTest test;

  auto sort_order = make_unique<SortOrder>();
  sort_order
      ->OrderByAttributeAt(0, ASCENDING)
      ->OrderByAttributeAt(1, ASCENDING);

  test.SetExpectedResult(n1n1a1_.Build());
  test.Execute(MergeUnionAll(
      std::move(sort_order),
      uptr_vector<Operation>(a1_.Build(), n1_.Build(), n1_.Build())));
}


TEST_F(MergeUnionAllTest, n1_a1_n1) {
  OperationTest test;

  auto sort_order = make_unique<SortOrder>();
  sort_order
      ->OrderByAttributeAt(0, ASCENDING)
      ->OrderByAttributeAt(1, ASCENDING);

  test.SetExpectedResult(n1n1a1_.Build());
  test.Execute(MergeUnionAll(
      std::move(sort_order),
      uptr_vector<Operation>(n1_.Build(), a1_.Build(), n1_.Build())));
}

TEST_F(MergeUnionAllTest, n1_n1_a1) {
  OperationTest test;

  auto sort_order = make_unique<SortOrder>();
  sort_order
      ->OrderByAttributeAt(0, ASCENDING)
      ->OrderByAttributeAt(1, ASCENDING);

  test.SetExpectedResult(n1n1a1_.Build());
  test.Execute(MergeUnionAll(
      std::move(sort_order),
      uptr_vector<Operation>(n1_.Build(), n1_.Build(), a1_.Build())));
}

TEST_F(MergeUnionAllTest, a1a1a2_b2) {
  OperationTest test;

  auto sort_order = make_unique<SortOrder>();
  sort_order
      ->OrderByAttributeAt(0, DESCENDING)
      ->OrderByAttributeAt(1, ASCENDING);

  test.SetExpectedResult(TestDataBuilder<STRING, INT64>()
      .AddRow("b", 2)
      .AddRow("a", 1)
      .AddRow("a", 1)
      .AddRow("a", 2)
      .Build());
  test.Execute(MergeUnionAll(
      std::move(sort_order),
      uptr_vector<Operation>(a1a1a2_.Build(), b2_.Build())));
}

TEST_F(MergeUnionAllTest, EmptySoftQuota) {
  MemoryLimit allocator_with_soft_quota(0, false, HeapBufferAllocator::Get());
  OperationTest test;

  auto sort_order = make_unique<SortOrder>();
  sort_order
      ->OrderByNamedAttribute("col0", ASCENDING)
      ->OrderByNamedAttribute("col1", ASCENDING);

  test.SetExpectedResult(TestDataBuilder<STRING, INT64>()
                         .AddRow("a", 1)
                         .AddRow("a", 2)
                         .Build());
  auto operation = MergeUnionAll(
      std::move(sort_order),
      uptr_vector<Operation>(a2_.Build(), a1_.Build()));
  operation->SetBufferAllocator(&allocator_with_soft_quota, true);
  test.Execute(std::move(operation));
}

TEST_F(MergeUnionAllTest, TransformTest) {
  auto input1 = a1a2b1b2_.BuildCursor();
  auto input2 = a1a3b2b2_.BuildCursor();

  auto input1_saved = input1.get();
  auto input2_saved = input2.get();

  auto sort_order = make_unique<SortOrder>();
  sort_order
      ->OrderByNamedAttribute("col0", ASCENDING)
      ->OrderByNamedAttribute("col1", ASCENDING);
  auto bound_sort_order = SucceedOrDie(sort_order->Bind(input1->schema()));

  std::unique_ptr<Cursor> merge(SucceedOrDie(BoundMergeUnionAll(
      std::move(bound_sort_order),
      uptr_vector<Cursor>(std::move(input1), std::move(input2)),
      HeapBufferAllocator::Get())));

  std::unique_ptr<CursorTransformerWithSimpleHistory> spy_transformer(
      PrintingSpyTransformer());
  merge->ApplyToChildren(spy_transformer.get());

  ASSERT_EQ(2, spy_transformer->GetHistoryLength());
  EXPECT_EQ(input1_saved, spy_transformer->GetEntryAt(0)->original());
  EXPECT_EQ(input2_saved, spy_transformer->GetEntryAt(1)->original());
}

}  // namespace supersonic
