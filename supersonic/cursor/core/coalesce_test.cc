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

#include "supersonic/cursor/core/coalesce.h"

#include <memory>

#include <glog/logging.h>
#include "gtest/gtest.h"
#include "supersonic/utils/logging-inl.h"
#include "supersonic/base/infrastructure/projector.h"
#include "supersonic/cursor/base/cursor.h"
#include "supersonic/cursor/base/cursor_transformer.h"
#include "supersonic/cursor/core/limit.h"
#include "supersonic/cursor/core/project.h"
#include "supersonic/cursor/core/spy.h"
#include "supersonic/proto/supersonic.pb.h"
#include "supersonic/testing/comparable_cursor.h"
#include "supersonic/testing/operation_testing.h"
#include "supersonic/utils/container_literal.h"
#include "supersonic/utils/functional.h"

using util::gtl::Container;

namespace supersonic {

class BoundCoalesceCursorTest : public ::testing::Test {
 public:
  static const int kAttributeCount = 4;
  static const int kRowCount = 16;

  void SetUp() {
    TestDataBuilder<INT32, INT32, INT32, INT32> builder;

    for (int i = 0; i < kRowCount; ++i) {
      const int row_tag = 10 * (i + 1);
      builder.AddRow(row_tag + 1, row_tag + 2, row_tag + 3, row_tag + 4);
    }

    test_data_= builder.Build();
  }

  unique_ptr<Cursor> CreateTestCursor() {
    return CreateTestCursor(0, kAttributeCount);
  }

  unique_ptr<Cursor> CreateTestCursor(int attribute_offset,
                           int attribute_count) {
    return CreateTestCursor(attribute_offset, attribute_count, kRowCount);
  }

  unique_ptr<Cursor> CreateTestCursor(int attribute_offset,
                           int attribute_count,
                           int row_count);

 private:
  std::unique_ptr<TestData> test_data_;
};

class BoundCoalesceCursorSpyTest : public BoundCoalesceCursorTest,
                                   public ::testing::WithParamInterface<bool> {
};

unique_ptr<Cursor> BoundCoalesceCursorTest::CreateTestCursor(int attribute_offset,
                                             int attribute_count,
                                             int row_count) {
  DCHECK_GE(attribute_offset, 0);
  DCHECK_GE(attribute_count, 0);
  DCHECK_LE(attribute_offset + attribute_count, kAttributeCount);
  DCHECK_GE(row_count, 0);
  DCHECK_LE(row_count, kRowCount);

  FailureOrOwned<Cursor> cursor = test_data_->CreateCursor();
  DCHECK(cursor.is_success());

  auto limit_cursor = BoundLimit(0, row_count, cursor.move());

  CompoundSingleSourceProjector projector;
  for (int i = attribute_offset;
       i < attribute_offset + attribute_count; i++) {
    projector.add(ProjectAttributeAt(i));
  }

  FailureOrOwned<const BoundSingleSourceProjector> bound_projector =
      projector.Bind(test_data_->schema());
  DCHECK(bound_projector.is_success());

  return BoundProject(bound_projector.move(), std::move(limit_cursor));
}

TEST_F(BoundCoalesceCursorTest, EmptyVector) {
  vector<unique_ptr<Cursor>> children;
  FailureOrOwned<Cursor> cursor = BoundCoalesce(std::move(children));
  EXPECT_TRUE(cursor.is_success());

  EXPECT_EQ(0, cursor->column_count());

  ResultView result_view = cursor->Next(Cursor::kDefaultRowCount);
  EXPECT_TRUE(result_view.is_eos());

  children.resize(0);
  children.emplace_back(cursor.move());
  FailureOrOwned<Cursor> nested_cursor = BoundCoalesce(std::move(children));
  EXPECT_TRUE(nested_cursor.is_success());

  EXPECT_EQ(0, nested_cursor->column_count());

  result_view = nested_cursor->Next(Cursor::kDefaultRowCount);
  EXPECT_TRUE(result_view.is_eos());
}

TEST_F(BoundCoalesceCursorTest, OneCursor) {
  vector<unique_ptr<Cursor>> children;
  children.emplace_back(CreateTestCursor());

  FailureOrOwned<Cursor> coalesced_cursor = BoundCoalesce(std::move(children));
  EXPECT_TRUE(coalesced_cursor.is_success());

  auto coalesce_result = make_unique<ComparableCursor>(coalesced_cursor.move());
  auto expected_result = make_unique<ComparableCursor>(CreateTestCursor());

  EXPECT_TRUE(*coalesce_result == *expected_result);
}

TEST_F(BoundCoalesceCursorTest, SameAttributeName) {
  vector<unique_ptr<Cursor>> children;
  children.emplace_back(CreateTestCursor(0, kAttributeCount - 1));
  children.emplace_back(CreateTestCursor(kAttributeCount - 2, 2));

  FailureOrOwned<Cursor> coalesced_cursor = BoundCoalesce(std::move(children));
  EXPECT_TRUE(coalesced_cursor.is_failure());
}

INSTANTIATE_TEST_CASE_P(SpyUse, BoundCoalesceCursorSpyTest, ::testing::Bool());

TEST_P(BoundCoalesceCursorSpyTest, SimpleCoalesce) {
  vector<unique_ptr<Cursor>> children;
  children.emplace_back(CreateTestCursor(0, kAttributeCount - 1));
  children.emplace_back(CreateTestCursor(kAttributeCount - 1, 1));

  std::unique_ptr<Cursor> coalesced_cursor(
      SucceedOrDie(BoundCoalesce(std::move(children))));

  if (GetParam()) {
    std::unique_ptr<CursorTransformerWithSimpleHistory> spy_transformer(
        PrintingSpyTransformer());
    coalesced_cursor->ApplyToChildren(spy_transformer.get());
    coalesced_cursor = spy_transformer->Transform(std::move(coalesced_cursor));
  }

  ComparableCursor coalesced_result(std::move(coalesced_cursor));
  ComparableCursor expected_result(CreateTestCursor());
  EXPECT_TRUE(coalesced_result == expected_result);
}

TEST_F(BoundCoalesceCursorTest, ManyCursors) {
  DCHECK_GE(kRowCount, kAttributeCount);
  vector<unique_ptr<Cursor>> children;
  for (int i = 0; i < kAttributeCount; i++) {
    children.emplace_back(CreateTestCursor(i, 1, kRowCount-i));
  }
  FailureOrOwned<Cursor> coalesced_cursor = BoundCoalesce(std::move(children));
  EXPECT_TRUE(coalesced_cursor.is_success());

  auto expected_cursor = CreateTestCursor(0, kAttributeCount, kRowCount - kAttributeCount + 1);

  ComparableCursor coalesced_result(coalesced_cursor.move());
  ComparableCursor expected_result(std::move(expected_cursor));
  EXPECT_TRUE(coalesced_result == expected_result);
}

TEST_F(BoundCoalesceCursorTest, EmptyCursor) {
  vector<unique_ptr<Cursor>> children;
  children.emplace_back(CreateTestCursor());
  children.emplace_back(CreateTestCursor(0, 0));

  FailureOrOwned<Cursor> coalesced_cursor = BoundCoalesce(std::move(children));
  EXPECT_TRUE(coalesced_cursor.is_success());

  auto expected_cursor = CreateTestCursor();

  ComparableCursor coalesced_result(coalesced_cursor.move());
  ComparableCursor expected_result(std::move(expected_cursor));
  EXPECT_TRUE(coalesced_result == expected_result);
}

TEST_F(BoundCoalesceCursorTest, RowCount) {
  vector<unique_ptr<Cursor>> children;
  children.emplace_back(CreateTestCursor(0, kAttributeCount - 1, kRowCount - 1));
  children.emplace_back(CreateTestCursor(kAttributeCount - 1, 1));

  FailureOrOwned<Cursor> coalesced_cursor = BoundCoalesce(std::move(children));
  EXPECT_TRUE(coalesced_cursor.is_success());

  auto expected_cursor = CreateTestCursor(0, kAttributeCount, kRowCount - 1);

  ComparableCursor coalesced_result(coalesced_cursor.move());
  ComparableCursor expected_result(std::move(expected_cursor));
  EXPECT_TRUE(coalesced_result == expected_result);
}

TEST_F(BoundCoalesceCursorTest, TransformTest) {
  vector<unique_ptr<Cursor>> children;
  children.emplace_back(CreateTestCursor(0, kAttributeCount - 1));
  children.emplace_back(CreateTestCursor(kAttributeCount - 1, 1));

  auto saved = map_container(children, [] (const unique_ptr<Cursor>& c) { return c.get(); });

  FailureOrOwned<Cursor> coalesced_cursor = BoundCoalesce(std::move(children));
  EXPECT_TRUE(coalesced_cursor.is_success());

  std::unique_ptr<CursorTransformerWithSimpleHistory> spy_transformer(
      PrintingSpyTransformer());
  coalesced_cursor->ApplyToChildren(spy_transformer.get());

  // Two input children.
  ASSERT_EQ(2, spy_transformer->GetHistoryLength());

  EXPECT_EQ(saved[0], spy_transformer->GetEntryAt(0)->original());
  EXPECT_EQ(saved[1], spy_transformer->GetEntryAt(1)->original());
}

TEST(CoalesceOperationTests, FailsOnDuplicatedColumnName) {
  vector<unique_ptr<Operation>> child_ops;
  TestDataBuilder<INT32, STRING> builder_1;
  builder_1.AddRow(0, "foo");
  child_ops.emplace_back(builder_1.Build());
  TestDataBuilder<INT32, STRING> builder_2;
  builder_2.AddRow(1, "bar");
  child_ops.emplace_back(builder_2.Build());
  auto op = Coalesce(std::move(child_ops));
  FailureOrOwned<Cursor> cursor(op->CreateCursor());
  EXPECT_TRUE(cursor.is_failure());
}

TEST(CoalesceOperationTests, Succeeds) {
  TestDataBuilder<INT32, STRING> builder_1;
  builder_1.AddRow(0, "foo");
  std::unique_ptr<Operation> op_1(Project(
      ProjectRename(Container("left_1", "left_2"), ProjectAllAttributes()),
      builder_1.Build()));
  TestDataBuilder<INT32, STRING> builder_2;
  builder_2.AddRow(1, "bar");
  std::unique_ptr<Operation> op_2(Project(
      ProjectRename(Container("right_1", "right_2"), ProjectAllAttributes()),
      builder_2.Build()));

  vector<unique_ptr<Operation>> children;
  children.emplace_back(std::move(op_1));
  children.emplace_back(std::move(op_2));

  auto op = Coalesce(std::move(children));

  FailureOrOwned<Cursor> cursor(op->CreateCursor());
  EXPECT_TRUE(cursor.is_success());
}

}  // namespace supersonic
