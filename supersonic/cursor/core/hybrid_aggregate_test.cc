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

#include <memory>

#include "supersonic/base/infrastructure/projector.h"
#include "supersonic/cursor/base/cursor.h"
#include "supersonic/cursor/base/cursor_transformer.h"
#include "supersonic/cursor/core/aggregate.h"
#include "supersonic/cursor/core/hybrid_group_utils.h"
#include "supersonic/cursor/core/sort.h"
#include "supersonic/cursor/core/spy.h"
#include "supersonic/cursor/infrastructure/ordering.h"
#include "supersonic/testing/block_builder.h"
#include "supersonic/testing/comparators.h"
#include "supersonic/testing/operation_testing.h"
#include "supersonic/testing/repeating_block.h"

#include "gtest/gtest.h"
#include "gtest/gtest.h"
#include "supersonic/utils/container_literal.h"

namespace supersonic {

namespace {

class HybridAggregateTest : public testing::Test {
 protected:
  CompoundSingleSourceProjector empty_projector_;
};

class HybridAggregateSpyTest : public testing::TestWithParam<bool> {};

// A lot of tests just copied from aggregate_groups_test.cc. Maybe it would be
// better to share this code somehow and avoid "copy & paste".

FailureOrOwned<Cursor> CreateGroupAggregate(
    const SingleSourceProjector& group_by,
    const AggregationSpecification& aggregation,
    unique_ptr<Cursor> input) {
  return BoundHybridGroupAggregate(
      group_by.Clone(),
      aggregation,
      "",
      HeapBufferAllocator::Get(),
      16,
      NULL,
      std::move(input));
}

// Sorts cursor according to all columns in an ascending order. This function is
// needed because group provides no guarantees over the order of returned rows,
// so output needs to be sorted before it is compared against expected output.
static unique_ptr<Cursor> Sort(unique_ptr<Cursor> input) {
  SortOrder sort_order;
  sort_order.add(ProjectAllAttributes(), ASCENDING);
  std::unique_ptr<const BoundSortOrder> bound_sort_order(
      SucceedOrDie(sort_order.Bind(input->schema())));
  std::unique_ptr<const SingleSourceProjector> result_projector(
      ProjectAllAttributes());
  std::unique_ptr<const BoundSingleSourceProjector> bound_result_projector(
      SucceedOrDie(result_projector->Bind(input->schema())));
  return unique_ptr<Cursor>(SucceedOrDie(BoundSort(
      std::move(bound_sort_order),
      std::move(bound_result_projector),
      std::numeric_limits<size_t>::max(),
      "",
      HeapBufferAllocator::Get(),
      std::move(input))));
}

TEST_F(HybridAggregateTest, SimpleAggregation) {
  auto input = TestDataBuilder<INT32>()
      .AddRow(1)
      .AddRow(3)
      .BuildCursor();
  AggregationSpecification aggregation;
  aggregation.AddAggregation(SUM, "col0", "sum");
  auto aggregate = SucceedOrDie(CreateGroupAggregate(empty_projector_, aggregation, std::move(input)));

  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<INT32>().AddRow(4).BuildCursor());
  EXPECT_EQ("sum", aggregate->schema().attribute(0).name());
  EXPECT_TRUE(aggregate->schema().attribute(0).is_nullable());
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(aggregate));
}

TEST_F(HybridAggregateTest, CountWithInputColumn) {
  auto input = TestDataBuilder<INT32>()
      .AddRow(1)
      .AddRow(3)
      .BuildCursor();
  AggregationSpecification aggregation;
  aggregation.AddAggregation(COUNT, "col0", "count");
  auto aggregate = SucceedOrDie(CreateGroupAggregate(empty_projector_, aggregation, std::move(input)));

  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<UINT64>().AddRow(2).BuildCursor());
  EXPECT_FALSE(aggregate->schema().attribute(0).is_nullable());
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(aggregate));
}

TEST_F(HybridAggregateTest, CountWithDefinedOutputType) {
  auto input = TestDataBuilder<INT32>()
      .AddRow(1)
      .AddRow(3)
      .BuildCursor();
  AggregationSpecification aggregation;
  aggregation.AddAggregationWithDefinedOutputType(COUNT, "col0", "count",
                                                  INT32);
  auto aggregate = SucceedOrDie(CreateGroupAggregate(empty_projector_, aggregation, std::move(input)));

  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<INT32>().AddRow(2).BuildCursor());
  EXPECT_FALSE(aggregate->schema().attribute(0).is_nullable());
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(aggregate));
}

TEST_F(HybridAggregateTest, CountWithNullableInputColumn) {
  auto input = TestDataBuilder<INT32>()
      .AddRow(1)
      .AddRow(__)
      .BuildCursor();
  AggregationSpecification aggregation;
  aggregation.AddAggregation(COUNT, "col0", "count");
  auto aggregate = SucceedOrDie(CreateGroupAggregate(empty_projector_, aggregation, std::move(input)));

  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<UINT64>().AddRow(1).BuildCursor());
  EXPECT_FALSE(aggregate->schema().attribute(0).is_nullable());
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(aggregate));
}

TEST_F(HybridAggregateTest, CountAll) {
  auto input = TestDataBuilder<INT32>()
      .AddRow(1)
      .AddRow(__)
      .BuildCursor();
  AggregationSpecification aggregation;
  aggregation.AddAggregation(COUNT, "", "count");
  auto aggregate = SucceedOrDie(CreateGroupAggregate(empty_projector_, aggregation, std::move(input)));

  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<UINT64>().AddRow(2).BuildCursor());
  EXPECT_FALSE(aggregate->schema().attribute(0).is_nullable());
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(aggregate));
}

// Three kinds of COUNT.
TEST_F(HybridAggregateTest, CountAllColumnDistinct) {
  auto input = TestDataBuilder<INT32>()
      .AddRow(1)
      .AddRow(1)
      .AddRow(__)
      .BuildCursor();
  AggregationSpecification aggregation;
  aggregation.AddAggregation(COUNT, "", "count_all");
  aggregation.AddAggregation(COUNT, "col0", "count_column");
  aggregation.AddDistinctAggregation(COUNT, "col0", "count_distinct");
  auto aggregate = SucceedOrDie(CreateGroupAggregate(empty_projector_, aggregation, std::move(input)));
  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<UINT64, UINT64, UINT64>().AddRow(3, 2, 1).BuildCursor());
  EXPECT_FALSE(aggregate->schema().attribute(0).is_nullable());
  EXPECT_FALSE(aggregate->schema().attribute(1).is_nullable());
  EXPECT_FALSE(aggregate->schema().attribute(2).is_nullable());
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(aggregate));
}

TEST_F(HybridAggregateTest, AggregationWithOnlyNullInputs) {
  auto input = TestDataBuilder<INT32>()
      .AddRow(__)
      .AddRow(__)
      .BuildCursor();
  AggregationSpecification aggregation;
  aggregation.AddAggregation(SUM, "col0", "sum");
  auto aggregate = SucceedOrDie(CreateGroupAggregate(empty_projector_, aggregation, std::move(input)));

  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<INT32>().AddRow(__).BuildCursor());
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(aggregate));
}

TEST_F(HybridAggregateTest, AggregationWithOutputTypeDifferentFromInputType) {
  auto input = TestDataBuilder<INT32>()
      .AddRow(1)
      .AddRow(3)
      .BuildCursor();
  AggregationSpecification aggregation;
  aggregation.AddAggregationWithDefinedOutputType(SUM, "col0", "sum", INT64);
  auto aggregate = SucceedOrDie(CreateGroupAggregate(empty_projector_, aggregation, std::move(input)));

  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<INT64>().AddRow(4).BuildCursor());
  EXPECT_EQ("sum", aggregate->schema().attribute(0).name());
  EXPECT_TRUE(aggregate->schema().attribute(0).is_nullable());
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(aggregate));
}

TEST_F(HybridAggregateTest, MultipleAggregations) {
  auto input = TestDataBuilder<INT32>()
      .AddRow(1)
      .AddRow(2)
      .AddRow(3)
      .BuildCursor();
  AggregationSpecification aggregation;
  aggregation.AddAggregation(SUM, "col0", "sum");
  aggregation.AddAggregation(MAX, "col0", "max");
  aggregation.AddAggregation(MIN, "col0", "min");
  auto aggregate = SucceedOrDie(CreateGroupAggregate(empty_projector_, aggregation, std::move(input)));

  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<INT32, INT32, INT32>().AddRow(6, 3, 1).BuildCursor());
  EXPECT_EQ("sum", aggregate->schema().attribute(0).name());
  EXPECT_EQ("max", aggregate->schema().attribute(1).name());
  EXPECT_EQ("min", aggregate->schema().attribute(2).name());
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(aggregate));
}

TEST_F(HybridAggregateTest, DistinctAggregation) {
  auto input = TestDataBuilder<INT32>()
      .AddRow(3)
      .AddRow(4)
      .AddRow(4)
      .AddRow(3)
      .BuildCursor();
  AggregationSpecification aggregation;
  aggregation.AddDistinctAggregation(SUM, "col0", "sum");
  auto aggregate = SucceedOrDie(CreateGroupAggregate(empty_projector_, aggregation, std::move(input)));

  // Only distinct values are summed (3 + 4).
  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<INT32>().AddRow(7).BuildCursor());
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(aggregate));
}

TEST_F(HybridAggregateTest, DistinctCountAggregation) {
  auto input = TestDataBuilder<INT32>()
      .AddRow(3)
      .AddRow(4)
      .AddRow(4)
      .AddRow(3)
      .BuildCursor();
  AggregationSpecification aggregation;
  aggregation.AddDistinctAggregationWithDefinedOutputType(
      COUNT, "col0", "count", INT32);
  auto aggregate = SucceedOrDie(CreateGroupAggregate(empty_projector_, aggregation, std::move(input)));
  EXPECT_EQ("count", aggregate->schema().attribute(0).name());
  // There are two distinct values (3, 4).
  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<INT32>().AddRow(2).BuildCursor());
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(aggregate));
}

TEST_F(HybridAggregateTest, DistinctCountAggregationNeedsInputColumn) {
  auto input = TestDataBuilder<INT32>()
      .AddRow(3)
      .BuildCursor();

  AggregationSpecification aggregation;
  // This should not work, there is no way to find distinct values if input
  // column is not specified.
  aggregation.AddDistinctAggregationWithDefinedOutputType(
      COUNT, "", "count", INT32);
  FailureOrOwned<Cursor> result(
      CreateGroupAggregate(empty_projector_, aggregation, std::move(input)));
  EXPECT_TRUE(result.is_failure());
}

TEST_F(HybridAggregateTest, AggregationWithGroupBy) {
  auto input = TestDataBuilder<INT32, INT32>()
      .AddRow(1, 3)
      .AddRow(3, -3)
      .AddRow(1, 4)
      .AddRow(3, -5)
      .BuildCursor();
  std::unique_ptr<const SingleSourceProjector> group_by_column(
      ProjectNamedAttribute("col0"));
  AggregationSpecification aggregation;
  aggregation.AddAggregation(SUM, "col1", "sum");
  auto aggregate = SucceedOrDie(CreateGroupAggregate(*group_by_column, aggregation, std::move(input)));

  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<INT32, INT32>().AddRow(1, 7).AddRow(3, -8).BuildCursor());

  EXPECT_EQ("col0", aggregate->schema().attribute(0).name());
  EXPECT_EQ("sum", aggregate->schema().attribute(1).name());
  EXPECT_FALSE(aggregate->schema().attribute(0).is_nullable());
  EXPECT_TRUE(aggregate->schema().attribute(1).is_nullable());
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(aggregate));
}

TEST_F(HybridAggregateTest, AggregationWithGroupByNullableColumn) {
  auto input = TestDataBuilder<INT32, INT32>()
      .AddRow(3, -3)
      .AddRow(__, 4)
      .AddRow(3, -5)
      .AddRow(__, 1)
      .BuildCursor();
  std::unique_ptr<const SingleSourceProjector> group_by_column(
      ProjectNamedAttribute("col0"));
  AggregationSpecification aggregation;
  aggregation.AddAggregation(SUM, "col1", "sum");
  auto aggregate = SucceedOrDie(CreateGroupAggregate(*group_by_column, aggregation, std::move(input)));
  std::unique_ptr<Cursor> expected_output(TestDataBuilder<INT32, INT32>()
                                              .AddRow(3, -8)
                                              .AddRow(__, 5)
                                              .BuildCursor());
  EXPECT_EQ("col0", aggregate->schema().attribute(0).name());
  EXPECT_EQ("sum", aggregate->schema().attribute(1).name());
  EXPECT_TRUE(aggregate->schema().attribute(0).is_nullable());
  EXPECT_TRUE(aggregate->schema().attribute(1).is_nullable());
  EXPECT_CURSORS_EQUAL(Sort(std::move(expected_output)), Sort(std::move(aggregate)));
}

INSTANTIATE_TEST_CASE_P(SpyUse, HybridAggregateSpyTest, testing::Bool());

TEST_P(HybridAggregateSpyTest, GroupBySecondColumn) {
  auto input = TestDataBuilder<INT32, STRING>()
      .AddRow(-3, "foo")
      .AddRow(2, "bar")
      .AddRow(3, "bar")
      .AddRow(-2, "foo")
      .BuildCursor();
  std::unique_ptr<const SingleSourceProjector> group_by_column(
      ProjectNamedAttribute("col1"));
  AggregationSpecification aggregation;
  aggregation.AddAggregation(SUM, "col0", "sum");
  auto aggregate = SucceedOrDie(CreateGroupAggregate(*group_by_column, aggregation, std::move(input)));

  if (GetParam()) {
    std::unique_ptr<CursorTransformerWithSimpleHistory> spy_transformer(
        PrintingSpyTransformer());
    aggregate->ApplyToChildren(spy_transformer.get());
    aggregate = spy_transformer->Transform(std::move(aggregate));
  }

  std::unique_ptr<Cursor> expected_output(TestDataBuilder<STRING, INT32>()
                                              .AddRow("foo", -5)
                                              .AddRow("bar", 5)
                                              .BuildCursor());
  EXPECT_EQ("col1", aggregate->schema().attribute(0).name());
  EXPECT_EQ("sum", aggregate->schema().attribute(1).name());
  EXPECT_CURSORS_EQUAL(Sort(std::move(expected_output)), Sort(std::move(aggregate)));
}

TEST_F(HybridAggregateTest, GroupByTwoColumns) {
  auto input = TestDataBuilder<STRING, INT32, INT32>()
      .AddRow("foo", 1, 3)
      .AddRow("bar", 2, -3)
      .AddRow("foo", 1, 4)
      .AddRow("bar", 3, -5)
      .BuildCursor();
  std::unique_ptr<const SingleSourceProjector> group_by_columns(
      ProjectNamedAttributes(util::gtl::Container("col0", "col1")));
  AggregationSpecification aggregation;
  aggregation.AddAggregation(SUM, "col2", "sum");
  auto aggregate = SucceedOrDie(
      CreateGroupAggregate(*group_by_columns, aggregation, std::move(input)));

  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<STRING, INT32, INT32>()
          .AddRow("foo", 1, 7)
          .AddRow("bar", 2, -3)
          .AddRow("bar", 3, -5)
          .BuildCursor());
  EXPECT_CURSORS_EQUAL(Sort(std::move(expected_output)), Sort(std::move(aggregate)));
}

TEST_F(HybridAggregateTest, GroupByTwoColumnsWithMultipleAggregations) {
  auto input = TestDataBuilder<STRING, INT32, INT32>()
      .AddRow("foo", 1, 3)
      .AddRow("bar", 2, -3)
      .AddRow("foo", 1, 4)
      .AddRow("bar", 3, -5)
      .BuildCursor();
  std::unique_ptr<const SingleSourceProjector> group_by_columns(
      ProjectNamedAttributes(util::gtl::Container("col0", "col1")));
  AggregationSpecification aggregation;
  aggregation.AddAggregation(SUM, "col2", "sum");
  aggregation.AddAggregation(MIN, "col2", "min");
  aggregation.AddAggregation(COUNT, "", "count");
  auto aggregate = SucceedOrDie(
      CreateGroupAggregate(*group_by_columns, aggregation, std::move(input)));
  EXPECT_EQ("count", aggregate->schema().attribute(4).name());
  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<STRING, INT32, INT32, INT32, UINT64>()
      // Group by col, group by col, SUM col, MIN col, COUNT col
          .AddRow("foo", 1, 7, 3, 2)
          .AddRow("bar", 2, -3, -3, 1)
          .AddRow("bar", 3, -5, -5, 1)
          .BuildCursor());
  EXPECT_CURSORS_EQUAL(Sort(std::move(expected_output)), Sort(std::move(aggregate)));
}

TEST_F(HybridAggregateTest, GroupByWithoutAggregateFunctions) {
  auto input = TestDataBuilder<STRING>()
      .AddRow("foo")
      .AddRow("bar")
      .AddRow("foo")
      .AddRow("bar")
      .BuildCursor();
  std::unique_ptr<const SingleSourceProjector> group_by_column(
      ProjectNamedAttribute("col0"));
  AggregationSpecification empty_aggregator;
  auto aggregate = SucceedOrDie(
      CreateGroupAggregate(*group_by_column, empty_aggregator, std::move(input)));

  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<STRING>().AddRow("foo").AddRow("bar").BuildCursor());
  EXPECT_CURSORS_EQUAL(Sort(std::move(expected_output)), Sort(std::move(aggregate)));
}

// Aggregation on empty input with empty key should return empty result.
TEST_F(HybridAggregateTest, AggregationOnEmptyInput) {
  auto input = TestDataBuilder<DATETIME>().BuildCursor();
  AggregationSpecification aggregation;
  aggregation.AddAggregation(MIN, "col0", "min");
  auto aggregate = SucceedOrDie(CreateGroupAggregate(empty_projector_, aggregation, std::move(input)));

  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<DATETIME>().BuildCursor());
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(aggregate));
}

// Aggregation on empty input with group by columns should return empty result.
TEST_F(HybridAggregateTest, AggregationOnEmptyInputWithGroupByColumn) {
  auto input = TestDataBuilder<STRING, DATETIME>().BuildCursor();
  std::unique_ptr<const SingleSourceProjector> group_by_column(
      ProjectNamedAttribute("col0"));
  AggregationSpecification aggregation;
  aggregation.AddAggregation(MIN, "col1", "min");
  auto aggregate = SucceedOrDie(CreateGroupAggregate(*group_by_column, aggregation, std::move(input)));

  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<STRING, DATETIME>().BuildCursor());
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(aggregate));
}

// Count on empty input with empty key should return empty result.
TEST_F(HybridAggregateTest, CountOnEmptyInput) {
  auto input = TestDataBuilder<DATETIME>().BuildCursor();
  AggregationSpecification aggregation;
  aggregation.AddAggregation(COUNT, "col0", "count");
  auto aggregate = SucceedOrDie(CreateGroupAggregate(empty_projector_, aggregation, std::move(input)));
  EXPECT_EQ("count", aggregate->schema().attribute(0).name());
  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<UINT64>().BuildCursor());
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(aggregate));
}

// Count on empty input with group by columns should return empty result.
TEST_F(HybridAggregateTest, CountOnEmptyInputWithGroupByColumn) {
  auto input = TestDataBuilder<STRING, DATETIME>()
      .BuildCursor();
  std::unique_ptr<const SingleSourceProjector> group_by_column(
      ProjectNamedAttribute("col0"));
  AggregationSpecification aggregation;
  aggregation.AddAggregation(COUNT, "col1", "count");
  auto aggregate = SucceedOrDie(CreateGroupAggregate(*group_by_column, aggregation, std::move(input)));
  EXPECT_EQ("count", aggregate->schema().attribute(1).name());
  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<STRING, UINT64>().BuildCursor());
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(aggregate));
}

TEST_F(HybridAggregateTest, AggregationInputColumnMissingError) {
  auto input = TestDataBuilder<INT32>().BuildCursor();
  AggregationSpecification aggregation;
  aggregation.AddAggregation(SUM, "NotExistingCol", "sum");
  FailureOrOwned<Cursor> result(
      CreateGroupAggregate(empty_projector_, aggregation, std::move(input)));
  ASSERT_TRUE(result.is_failure());
  EXPECT_EQ(ERROR_ATTRIBUTE_MISSING, result.exception().return_code());
}

TEST_F(HybridAggregateTest, AggregationResultColumnExistsError) {
  auto input = TestDataBuilder<INT32, INT32>().BuildCursor();
  AggregationSpecification aggregation;
  // Two results can not be stored in the same column.
  aggregation.AddAggregation(SUM, "col0", "result_col");
  aggregation.AddAggregation(MIN, "col1", "result_col");
  FailureOrOwned<Cursor> result(
      CreateGroupAggregate(empty_projector_, aggregation, std::move(input)));
  ASSERT_TRUE(result.is_failure());
  EXPECT_EQ(ERROR_ATTRIBUTE_EXISTS, result.exception().return_code());
}

TEST_F(HybridAggregateTest, NotSupportedAggregationError) {
  auto input = TestDataBuilder<BINARY>().BuildCursor();
  AggregationSpecification aggregation;
  aggregation.AddAggregation(SUM, "col0", "sum");
  FailureOrOwned<Cursor> result(
      CreateGroupAggregate(empty_projector_, aggregation, std::move(input)));
  ASSERT_TRUE(result.is_failure());
  EXPECT_EQ(ERROR_INVALID_ARGUMENT_TYPE, result.exception().return_code());
}

TEST_F(HybridAggregateTest, ExceptionFromInputPropagated) {
  auto input = TestDataBuilder<INT32>()
      .ReturnException(ERROR_GENERAL_IO_ERROR)
      .BuildCursor();
  AggregationSpecification aggregation;
  aggregation.AddAggregation(COUNT, "col0", "count");
  FailureOrOwned<Cursor> cursor(
      CreateGroupAggregate(empty_projector_, aggregation, std::move(input)));
  ASSERT_TRUE(cursor.is_success());
  ResultView result = cursor.get()->Next(100);
  ASSERT_TRUE(result.is_failure());
  EXPECT_EQ(ERROR_GENERAL_IO_ERROR, result.exception().return_code());
}

TEST_F(HybridAggregateTest, LargeInput) {
  TestDataBuilder<INT64, STRING> cursor_builder;
  for (int i = 0; i < 3 * Cursor::kDefaultRowCount + 1; ++i) {
    cursor_builder.AddRow(13, "foo")
        .AddRow(17, "bar")
        .AddRow(13, "foo");
  }
  auto input = cursor_builder.BuildCursor();

  std::unique_ptr<const SingleSourceProjector> group_by_columns(
      ProjectNamedAttributes(util::gtl::Container("col0", "col1")));
  AggregationSpecification aggregation;
  aggregation.AddAggregation(SUM, "col0", "sum");
  auto aggregate = SucceedOrDie(
      CreateGroupAggregate(*group_by_columns, aggregation, std::move(input)));

  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<INT64, STRING, INT64>()
          .AddRow(13, "foo", 2 * 13 * (3 * Cursor::kDefaultRowCount + 1))
          .AddRow(17, "bar", 17 * (3 * Cursor::kDefaultRowCount + 1))
          .BuildCursor());
  EXPECT_CURSORS_EQUAL(Sort(std::move(expected_output)), Sort(std::move(aggregate)));
}

TEST_F(HybridAggregateTest, TransformTest) {
  auto input = TestDataBuilder<DATETIME>().BuildCursor();
  AggregationSpecification aggregation;
  aggregation.AddAggregation(MIN, "col0", "min");
  auto aggregate = SucceedOrDie(CreateGroupAggregate(empty_projector_, aggregation, std::move(input)));

  std::unique_ptr<CursorTransformerWithSimpleHistory> spy_transformer(
      PrintingSpyTransformer());
  aggregate->ApplyToChildren(spy_transformer.get());

  // Spy transformer should add one child, it will be a transformed version of
  // the input cursor.
  ASSERT_EQ(1, spy_transformer->GetHistoryLength());
}

// Some new tests. To be removed if the duplicated the tests above.

TEST_F(HybridAggregateTest, NoGroupByColumns) {
  OperationTest test;
  test.SetInput(TestDataBuilder<INT32>()
                .AddRow(1)
                .AddRow(1)
                .AddRow(3)
                .AddRow(3)
                .AddRow(2)
                .AddRow(3)
                .AddRow(1)
                .Build());
  test.SetExpectedResult(TestDataBuilder<INT32, UINT64, UINT64>()
                         .AddRow(14, 7, 3)
                         .Build());
  auto aggregation = make_unique<AggregationSpecification>();
  aggregation->AddAggregation(SUM, "col0", "sum");
  aggregation->AddAggregation(COUNT, "col0", "cnt");
  aggregation->AddDistinctAggregation(COUNT, "col0", "dcnt");
  test.Execute(HybridGroupAggregate(
      empty_projector_.Clone(),
      std::move(aggregation),
      16,
      "",
      test.input()));
}

TEST_F(HybridAggregateTest, Simple1) {
  OperationTest test;
  test.SetIgnoreRowOrder(true);
  test.SetInput(
      TestDataBuilder<INT32, INT32>()
      .AddRow(1, 3)
      .AddRow(1, 4)
      .AddRow(3, -3)
      .AddRow(2, 4)
      .AddRow(3, -5)
      .Build());
  test.SetExpectedResult(
      TestDataBuilder<INT32, INT32, INT32, UINT64, UINT64>()
      .AddRow(1, 7, 2, 2, 1)
      .AddRow(2, 4, 2, 1, 1)
      .AddRow(3, -8, 6, 2, 1)
      .Build());

  std::unique_ptr<const SingleSourceProjector> group_by_columns(
      ProjectNamedAttribute("col0"));
  auto aggregation = make_unique<AggregationSpecification>();
  aggregation->AddAggregation(SUM, "col1", "sum");
  aggregation->AddAggregation(SUM, "col0", "sum2");
  aggregation->AddAggregation(COUNT, "col0", "cnt");
  aggregation->AddDistinctAggregation(COUNT, "col0", "dcnt");
  test.Execute(HybridGroupAggregate(
      std::move(group_by_columns),
      std::move(aggregation),
      16,
      "",
      test.input()));
}

TEST_F(HybridAggregateTest, DistinctAggregations) {
  OperationTest test;
  test.SetInput(TestDataBuilder<INT32, INT32>()
                .AddRow(1, 3)
                .AddRow(1, 4)
                .AddRow(3, -1)
                .AddRow(3, -2)
                .AddRow(2, 4)
                .AddRow(3, -3)
                .AddRow(1, 3)
                .Build());
  test.SetExpectedResult(TestDataBuilder<INT32, INT32, UINT64, UINT64>()
                         .AddRow(1, 7, 2, 1)
                         .AddRow(2, 4, 1, 1)
                         .AddRow(3, -6, 3, 1)
                         .Build());
  std::unique_ptr<const SingleSourceProjector> group_by_columns(
      ProjectNamedAttribute("col0"));
  auto aggregation = make_unique<AggregationSpecification>();
  aggregation->AddDistinctAggregation(SUM, "col1", "sum");
  aggregation->AddDistinctAggregation(COUNT, "col1", "cnt");
  aggregation->AddDistinctAggregation(COUNT, "col0", "cnt2");
  test.Execute(HybridGroupAggregate(
      std::move(group_by_columns),
      std::move(aggregation),
      16,
      "",
      test.input()));
}

TEST_F(HybridAggregateTest, NonDistinctAndDistinctAggregations) {
  OperationTest test;
  test.SetInput(TestDataBuilder<INT32, INT32>()
                .AddRow(1, 3)
                .AddRow(1, 4)
                .AddRow(3, -1)
                .AddRow(3, -2)
                .AddRow(2, 4)
                .AddRow(3, -3)
                .AddRow(1, 3)
                .AddRow(1, __)
                .Build());
  test.SetExpectedResult(TestDataBuilder<INT32, INT32, UINT64, INT32, UINT64,
                                         UINT64>()
                         .AddRow(1, 7, 2, 10, 3, 4)
                         .AddRow(2, 4, 1, 4, 1, 1)
                         .AddRow(3, -6, 3, -6, 3, 3)
                         .Build());
  std::unique_ptr<const SingleSourceProjector> group_by_columns(
      ProjectNamedAttribute("col0"));
  auto aggregation = make_unique<AggregationSpecification>();
  aggregation->AddDistinctAggregation(SUM, "col1", "sum");
  aggregation->AddDistinctAggregation(COUNT, "col1", "cnt");
  aggregation->AddAggregation(SUM, "col1", "sum2");
  aggregation->AddAggregation(COUNT, "col1", "cnt2");
  aggregation->AddAggregation(COUNT, "", "cnt3");
  test.Execute(HybridGroupAggregate(
      std::move(group_by_columns),
      std::move(aggregation),
      16,
      "",
      test.input()));
}

// Test hybrid group transform for two distinct aggregations some non-distinct
// aggregations including COUNT(*). Each distinct aggregation should get its
// copy of data. All non-distinct aggregation share one copy of the data.
// COUNT(*) gets a new column in the non-distinct data.
TEST_F(HybridAggregateTest, HybridGroupTransformTest) {
  std::unique_ptr<Cursor> input(TestDataBuilder<INT32, INT32, INT32>()
                                    .AddRow(1, 3, 1)
                                    .AddRow(1, 4, 2)
                                    .AddRow(3, -3, 3)
                                    .AddRow(2, 4, 4)
                                    .AddRow(3, -5, 5)
                                    .BuildCursor());
  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<INT32, INT32, INT32, INT32, INT32, INT32>()
          .AddRow(1, 3, __, __, __, __)
          .AddRow(1, 4, __, __, __, __)
          .AddRow(3, -3, __, __, __, __)
          .AddRow(2, 4, __, __, __, __)
          .AddRow(3, -5, __, __, __, __)
          .AddRow(1, __, 1, __, __, __)
          .AddRow(1, __, 2, __, __, __)
          .AddRow(3, __, 3, __, __, __)
          .AddRow(2, __, 4, __, __, __)
          .AddRow(3, __, 5, __, __, __)
          .AddRow(1, __, __, 3, 1, 0)
          .AddRow(1, __, __, 4, 1, 0)
          .AddRow(3, __, __, -3, 3, 0)
          .AddRow(2, __, __, 4, 2, 0)
          .AddRow(3, __, __, -5, 3, 0)
          .BuildCursor());
  std::unique_ptr<const SingleSourceProjector> group_by_columns(
      ProjectNamedAttribute("col0"));
  AggregationSpecification aggregation;
  aggregation.AddAggregation(SUM, "col1", "sum");
  aggregation.AddAggregation(SUM, "col0", "sum2");
  aggregation.AddAggregation(COUNT, "col0", "cnt");
  aggregation.AddAggregation(COUNT, "", "cnt_star");
  aggregation.AddAggregation(COUNT, "", "cnt_star2");
  aggregation.AddDistinctAggregation(COUNT, "col1", "dcnt1");
  aggregation.AddDistinctAggregation(SUM, "col2", "dsum2");
  aggregation.AddDistinctAggregation(COUNT, "col2", "dcnt2");

  auto transformed = SucceedOrDie(
      BoundHybridGroupAggregate(
          std::move(group_by_columns),
          aggregation,
          "",
          HeapBufferAllocator::Get(),
          0,
          (new HybridGroupDebugOptions)->set_return_transformed_input(true),
          std::move(input)));
  EXPECT_FALSE(transformed->schema().attribute(0).is_nullable());
  EXPECT_TRUE(transformed->schema().attribute(1).is_nullable());
  EXPECT_TRUE(transformed->schema().attribute(2).is_nullable());
  EXPECT_TRUE(transformed->schema().attribute(3).is_nullable());
  EXPECT_TRUE(transformed->schema().attribute(4).is_nullable());
  EXPECT_TRUE(transformed->schema().attribute(5).is_nullable());
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(transformed));
}

TEST_F(HybridAggregateTest, HybridGroupTransformOnlyDistinct) {
  std::unique_ptr<Cursor> input(TestDataBuilder<INT32, INT32, INT32>()
                                    .AddRow(1, 3, 10)
                                    .AddRow(1, 4, 11)
                                    .AddRow(3, -3, 10)
                                    .AddRow(2, 4, 11)
                                    .AddRow(3, -5, 10)
                                    .BuildCursor());
  std::unique_ptr<Cursor> expected_output(TestDataBuilder<INT32, INT32, INT32>()
                                              .AddRow(1, 3, __)
                                              .AddRow(1, 4, __)
                                              .AddRow(3, -3, __)
                                              .AddRow(2, 4, __)
                                              .AddRow(3, -5, __)
                                              .AddRow(1, __, 10)
                                              .AddRow(1, __, 11)
                                              .AddRow(3, __, 10)
                                              .AddRow(2, __, 11)
                                              .AddRow(3, __, 10)
                                              .BuildCursor());
  std::unique_ptr<const SingleSourceProjector> group_by_columns(
      ProjectNamedAttribute("col0"));
  AggregationSpecification aggregation;
  aggregation.AddDistinctAggregation(COUNT, "col1", "distinct1");
  aggregation.AddDistinctAggregation(COUNT, "col2", "distinct2");
  aggregation.AddDistinctAggregation(SUM, "col1", "distinct1b");
  aggregation.AddDistinctAggregation(COUNT, "col1", "distinct1c");
  std::unique_ptr<Cursor> transformed(SucceedOrDie(BoundHybridGroupAggregate(
      std::move(group_by_columns), aggregation, "", HeapBufferAllocator::Get(),
      0, (new HybridGroupDebugOptions)->set_return_transformed_input(true),
      std::move(input))));
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(transformed));
}

TEST_F(HybridAggregateTest, HybridGroupTransformDistinctAndCountAll) {
  std::unique_ptr<Cursor> input(TestDataBuilder<INT32, INT32, INT32>()
                                    .AddRow(1, 3, 10)
                                    .AddRow(1, 4, 11)
                                    .AddRow(3, -3, 10)
                                    .AddRow(2, 4, 11)
                                    .AddRow(3, -5, 10)
                                    .BuildCursor());
  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<INT32, INT32, INT32, INT32>()
          .AddRow(1, 3, __, __)
          .AddRow(1, 4, __, __)
          .AddRow(3, -3, __, __)
          .AddRow(2, 4, __, __)
          .AddRow(3, -5, __, __)
          .AddRow(1, __, 10, __)
          .AddRow(1, __, 11, __)
          .AddRow(3, __, 10, __)
          .AddRow(2, __, 11, __)
          .AddRow(3, __, 10, __)
          .AddRow(1, __, __, 0)
          .AddRow(1, __, __, 0)
          .AddRow(3, __, __, 0)
          .AddRow(2, __, __, 0)
          .AddRow(3, __, __, 0)
          .BuildCursor());
  std::unique_ptr<const SingleSourceProjector> group_by_columns(
      ProjectNamedAttribute("col0"));
  AggregationSpecification aggregation;
  aggregation.AddDistinctAggregation(COUNT, "col1", "distinct1");
  aggregation.AddDistinctAggregation(COUNT, "col2", "distinct2");
  aggregation.AddDistinctAggregation(SUM, "col1", "distinct1b");
  aggregation.AddDistinctAggregation(COUNT, "col1", "distinct1c");
  aggregation.AddAggregation(COUNT, "", "count_all");
  std::unique_ptr<Cursor> transformed(SucceedOrDie(BoundHybridGroupAggregate(
      std::move(group_by_columns), aggregation, "", HeapBufferAllocator::Get(),
      0, (new HybridGroupDebugOptions)->set_return_transformed_input(true),
      std::move(input))));
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(transformed));
}

// The example from implementation comment.
TEST_F(HybridAggregateTest, HybridGroupTransformExampleTest) {
  std::unique_ptr<Cursor> input(
      TestDataBuilder<INT32, INT32, INT32, INT32, INT32>()
          .AddRow(1, 2, 3, 4, 5)
          .AddRow(6, 7, 8, 9, 0)
          .BuildCursor());
  std::unique_ptr<Cursor> expected_output(
      TestDataBuilder<INT32, INT32, INT32, INT32, INT32>()
          .AddRow(1, 2, __, __, __)
          .AddRow(6, 7, __, __, __)
          .AddRow(1, __, 3, __, __)
          .AddRow(6, __, 8, __, __)
          .AddRow(1, __, __, 4, 2)
          .AddRow(6, __, __, 9, 7)
          .BuildCursor());
  std::unique_ptr<const SingleSourceProjector> group_by_columns(
      ProjectNamedAttribute("col0"));
  AggregationSpecification aggregation;
  aggregation.AddAggregation(SUM, "col3", "sum2");
  aggregation.AddAggregation(SUM, "col1", "sum");
  aggregation.AddDistinctAggregation(COUNT, "col1", "dcnt1");
  aggregation.AddDistinctAggregation(COUNT, "col2", "dcnt2");

  std::unique_ptr<Cursor> transformed(SucceedOrDie(BoundHybridGroupAggregate(
      std::move(group_by_columns), aggregation, "", HeapBufferAllocator::Get(),
      0, (new HybridGroupDebugOptions)->set_return_transformed_input(true),
      std::move(input))));
  EXPECT_FALSE(transformed->schema().attribute(0).is_nullable());
  EXPECT_TRUE(transformed->schema().attribute(1).is_nullable());
  EXPECT_TRUE(transformed->schema().attribute(2).is_nullable());
  EXPECT_TRUE(transformed->schema().attribute(3).is_nullable());
  EXPECT_TRUE(transformed->schema().attribute(4).is_nullable());
  EXPECT_CURSORS_EQUAL(std::move(expected_output), std::move(transformed));
}

// Several tests that check that hybrid group's group by columns can be
// specified by various kinds of projectors (by position, all columns, renaming
// projectors).
// TODO(user): It would be useful to be able to assert which branch of the
// hybrid group algorithm was chosen (three branches that differ in the number
// of projections applied), so we could make sure we are testing them all...
TEST_F(HybridAggregateTest, GroupByColumnPosition1) {
  OperationTest test;
  test.SetInput(TestDataBuilder<INT32, INT32>()
                .AddRow(1, 0)
                .AddRow(1, 0)
                .AddRow(3, 0)
                .AddRow(3, 0)
                .AddRow(2, 0)
                .AddRow(3, 0)
                .AddRow(1, 0)
                .Build());
  test.SetExpectedResult(TestDataBuilder<INT32, INT32, UINT64, UINT64>()
                         .AddRow(0, 14, 7, 3)
                         .Build());
  auto aggregation = make_unique<AggregationSpecification>();
  aggregation->AddAggregation(SUM, "col0", "sum");
  aggregation->AddAggregation(COUNT, "col0", "cnt");
  aggregation->AddDistinctAggregation(COUNT, "col0", "dcnt");
  test.Execute(HybridGroupAggregate(
      ProjectAttributeAt(1),
      std::move(aggregation),
      16,
      "",
      test.input()));
}

TEST_F(HybridAggregateTest, GroupByAllColumns) {
  OperationTest test;
  test.SetInput(TestDataBuilder<INT32>()
                .AddRow(1)
                .AddRow(1)
                .AddRow(3)
                .AddRow(3)
                .AddRow(2)
                .AddRow(3)
                .AddRow(1)
                .Build());
  test.SetExpectedResult(TestDataBuilder<INT32, INT32, UINT64, UINT64>()
                         .AddRow(1, 3, 3, 1)
                         .AddRow(2, 2, 1, 1)
                         .AddRow(3, 9, 3, 1)
                         .Build());
  auto aggregation = make_unique<AggregationSpecification>();
  aggregation->AddAggregation(SUM, "col0", "sum");
  aggregation->AddAggregation(COUNT, "col0", "cnt");
  aggregation->AddDistinctAggregation(COUNT, "col0", "dcnt");
  test.Execute(HybridGroupAggregate(
      ProjectAllAttributes(),
      std::move(aggregation),
      16,
      "",
      test.input()));
}

TEST_F(HybridAggregateTest, GroupByColumnRenamed) {
  OperationTest test;
  test.SetInput(TestDataBuilder<INT32, INT32>()
                .AddRow(1, 0)
                .AddRow(1, 0)
                .AddRow(3, 0)
                .AddRow(3, 0)
                .AddRow(2, 0)
                .AddRow(3, 0)
                .AddRow(1, 0)
                .Build());
  test.SetExpectedResult(TestDataBuilder<INT32, INT32, UINT64, UINT64>()
                         .AddRow(0, 14, 7, 3)
                         .Build());
  auto aggregation = make_unique<AggregationSpecification>();
  aggregation->AddAggregation(SUM, "col0", "sum");
  aggregation->AddAggregation(COUNT, "col0", "cnt");
  aggregation->AddDistinctAggregation(COUNT, "col0", "dcnt");
  test.Execute(HybridGroupAggregate(
      ProjectNamedAttributeAs("col1", "key"),
      std::move(aggregation),
      16,
      "",
      test.input()));
}

}  // namespace

}  // namespace supersonic
