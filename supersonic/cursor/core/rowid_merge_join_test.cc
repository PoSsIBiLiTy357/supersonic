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

#include "supersonic/cursor/core/rowid_merge_join.h"

#include <memory>

#include "supersonic/utils/stl_util.h"
#include "supersonic/base/infrastructure/projector.h"
#include "supersonic/base/infrastructure/types.h"
#include "supersonic/cursor/base/cursor.h"
#include "supersonic/cursor/base/cursor_transformer.h"
#include "supersonic/cursor/core/spy.h"
#include "supersonic/proto/supersonic.pb.h"
#include "supersonic/testing/block_builder.h"
#include "supersonic/testing/comparators.h"
#include "supersonic/testing/operation_testing.h"
#include "gtest/gtest.h"
#include "supersonic/utils/container_literal.h"

namespace supersonic {

class RowidMergeJoinTest : public testing::Test {
 protected:
  virtual void CreateSampleData() {
    sample_input_1_.AddRow(0, "A")
                   .AddRow(1, "B")
                   .AddRow(3, "D");
    sample_input_2_.AddRow("AA")
                   .AddRow("BB")
                   .AddRow("CC")
                   .AddRow("DD");
    sample_output_.AddRow("A", "AA")
                  .AddRow("B", "BB")
                  .AddRow("D", "DD");
  }
  TestDataBuilder<kRowidDatatype, STRING> sample_input_1_;
  TestDataBuilder<STRING> sample_input_2_;
  TestDataBuilder<STRING, STRING> sample_output_;
};

unique_ptr<Cursor> CreateRowidMergeJoin(
    const SingleSourceProjector& left_key,
    const MultiSourceProjector& result_projector,
    unique_ptr<Cursor> left,
    unique_ptr<Cursor> right) {
  auto bound_left = left_key.Bind(left->schema());
  CHECK(bound_left.is_success());

  auto bound_right = result_projector.Bind({left->schema(), right->schema()});
  CHECK(bound_right.is_success());

  return BoundRowidMergeJoin(
      bound_left.move(),
      bound_right.move(),
      std::move(left),
      std::move(right),
      HeapBufferAllocator::Get());
}

TEST_F(RowidMergeJoinTest, OneToOne) {
  OperationTest test;
  test.AddInput(TestDataBuilder<kRowidDatatype, STRING>()
                .AddRow(0, "A")
                .AddRow(1, "B")
                .AddRow(2, "C")
                .AddRow(3, "D")
                .AddRow(4, "E")
                .AddRow(5, "F")
                .Build());
  test.AddInput(TestDataBuilder<STRING>()
                .AddRow("AA")
                .AddRow("BB")
                .AddRow("CC")
                .AddRow("DD")
                .AddRow("EE")
                .AddRow("FF")
                .Build());
  test.SetExpectedResult(TestDataBuilder<STRING, STRING>()
                         .AddRow("A", "AA")
                         .AddRow("B", "BB")
                         .AddRow("C", "CC")
                         .AddRow("D", "DD")
                         .AddRow("E", "EE")
                         .AddRow("F", "FF")
                         .Build());
  auto proj = make_unique<CompoundMultiSourceProjector>();
  proj->add(0, ProjectAttributeAtAs(1, "col0"))
      ->add(1, ProjectAttributeAtAs(0, "col1"));
  test.Execute(
      RowidMergeJoin(
          ProjectAttributeAt(0),
          std::move(proj),
          test.input_at(0),
          test.input_at(1)));
}

TEST_F(RowidMergeJoinTest, OneToZeroOrOne) {
  CreateSampleData();
  OperationTest test;
  test.AddInput(sample_input_1_.Build());
  test.AddInput(sample_input_2_.Build());
  test.SetExpectedResult(sample_output_.Build());
  auto proj = make_unique<CompoundMultiSourceProjector>();
  proj->add(0, ProjectAttributeAtAs(1, "col0"))
      ->add(1, ProjectAttributeAtAs(0, "col1"));
  test.Execute(
      RowidMergeJoin(
          ProjectAttributeAt(0),
          std::move(proj),
          test.input_at(0),
          test.input_at(1)));
}

TEST_F(RowidMergeJoinTest, OneToZeroOrOneWithSpyTransform) {
  CreateSampleData();
  auto input1 = sample_input_1_.BuildCursor();
  auto input2 = sample_input_2_.BuildCursor();
  auto expected_result = sample_output_.BuildCursor();

  std::unique_ptr<const SingleSourceProjector> left_project(
      ProjectAttributeAt(0));
  auto right_project = make_unique<CompoundMultiSourceProjector>();
  right_project->add(0, ProjectAttributeAtAs(1, "col0"));
  right_project->add(1, ProjectAttributeAtAs(0, "col1"));

  auto rowid_merge = CreateRowidMergeJoin(*left_project, *right_project,
                                          std::move(input1), std::move(input2));

  std::unique_ptr<CursorTransformerWithSimpleHistory> spy_transformer(
      PrintingSpyTransformer());
  rowid_merge->ApplyToChildren(spy_transformer.get());
  rowid_merge = spy_transformer->Transform(std::move(rowid_merge));

  EXPECT_CURSORS_EQUAL(std::move(expected_result), std::move(rowid_merge));
}

TEST_F(RowidMergeJoinTest, OneToMany) {
  OperationTest test;
  test.AddInput(TestDataBuilder<kRowidDatatype, STRING>()
                .AddRow(0, "A1")
                .AddRow(0, "A2")
                .AddRow(0, "A3")
                .AddRow(0, "A4")
                .AddRow(0, "A5")
                .AddRow(1, "B1")
                .AddRow(1, "B2")
                .AddRow(1, "B3")
                .AddRow(1, "B4")
                .AddRow(1, "B5")
                .AddRow(3, "D1")
                .AddRow(3, "D2")
                .AddRow(3, "D3")
                .AddRow(3, "D4")
                .AddRow(3, "D5")
                .AddRow(3, "D6")
                .Build());
  test.AddInput(TestDataBuilder<STRING>()
                .AddRow("AA")
                .AddRow("BB")
                .AddRow("CC")
                .AddRow("DD")
                .Build());
  test.SetExpectedResult(TestDataBuilder<STRING, STRING>()
                         .AddRow("A1", "AA")
                         .AddRow("A2", "AA")
                         .AddRow("A3", "AA")
                         .AddRow("A4", "AA")
                         .AddRow("A5", "AA")
                         .AddRow("B1", "BB")
                         .AddRow("B2", "BB")
                         .AddRow("B3", "BB")
                         .AddRow("B4", "BB")
                         .AddRow("B5", "BB")
                         .AddRow("D1", "DD")
                         .AddRow("D2", "DD")
                         .AddRow("D3", "DD")
                         .AddRow("D4", "DD")
                         .AddRow("D5", "DD")
                         .AddRow("D6", "DD")
                         .Build());
  auto proj = make_unique<CompoundMultiSourceProjector>();
  proj->add(0, ProjectAttributeAtAs(1, "col0"))
      ->add(1, ProjectAttributeAtAs(0, "col1"));
  test.Execute(
      RowidMergeJoin(
          ProjectAttributeAt(0),
          std::move(proj),
          test.input_at(0),
          test.input_at(1)));
}

TEST_F(RowidMergeJoinTest, OneToManyWithNulls) {
  OperationTest test;
  test.AddInput(TestDataBuilder<kRowidDatatype, STRING>()
                .AddRow(0, "A1")
                .AddRow(0, "A2")
                .AddRow(0, "A3")
                .AddRow(0, "A4")
                .AddRow(0, "A5")
                .AddRow(1, "B1")
                .AddRow(1, "B2")
                .AddRow(1, "B3")
                .AddRow(1, "B4")
                .AddRow(1, "B5")
                .AddRow(3, __)
                .AddRow(3, "D2")
                .AddRow(3, "D3")
                .AddRow(3, __)
                .AddRow(3, "D5")
                .AddRow(3, "D6")
                .Build());
  test.AddInput(TestDataBuilder<STRING>()
                .AddRow("AA")
                .AddRow(__)
                .AddRow("CC")
                .AddRow("DD")
                .Build());
  test.SetExpectedResult(TestDataBuilder<STRING, STRING>()
                         .AddRow("A1", "AA")
                         .AddRow("A2", "AA")
                         .AddRow("A3", "AA")
                         .AddRow("A4", "AA")
                         .AddRow("A5", "AA")
                         .AddRow("B1", __)
                         .AddRow("B2", __)
                         .AddRow("B3", __)
                         .AddRow("B4", __)
                         .AddRow("B5", __)
                         .AddRow(__  , "DD")
                         .AddRow("D2", "DD")
                         .AddRow("D3", "DD")
                         .AddRow(__  , "DD")
                         .AddRow("D5", "DD")
                         .AddRow("D6", "DD")
                         .Build());
  auto proj = make_unique<CompoundMultiSourceProjector>();
  proj->add(0, ProjectAttributeAtAs(1, "col0"))
      ->add(1, ProjectAttributeAtAs(0, "col1"));
  test.Execute(
      RowidMergeJoin(
          ProjectAttributeAt(0),
          std::move(proj),
          test.input_at(0),
          test.input_at(1)));
}

TEST_F(RowidMergeJoinTest, ReferentialIntegrity) {
  OperationTest test;
  test.AddInput(TestDataBuilder<kRowidDatatype, STRING>()
                .AddRow(0, "A")
                .AddRow(1, "B1")
                .AddRow(1, "B2")
                .AddRow(1, "B3")
                .AddRow(3, "D1")
                .AddRow(3, "D2")
                .AddRow(3, "D3")
                .Build());
  test.AddInput(TestDataBuilder<STRING>()
                .AddRow("AA")
                .AddRow("BB")
                .AddRow("CC")
                .Build());
  test.SetExpectedResult(
      TestDataBuilder<STRING, STRING>()
      .AddRow("A", "AA")
      .AddRow("B1", "BB")
      .AddRow("B2", "BB")
      .AddRow("B3", "BB")
      .ReturnException(ERROR_FOREIGN_KEY_INVALID)
      .Build());
  auto proj = make_unique<CompoundMultiSourceProjector>();
  proj->add(0, ProjectAttributeAtAs(1, "col0"))
      ->add(1, ProjectAttributeAtAs(0, "col1"));
  test.Execute(
      RowidMergeJoin(
          ProjectAttributeAt(0),
          std::move(proj),
          test.input_at(0),
          test.input_at(1)));
}

TEST_F(RowidMergeJoinTest, TransformTest) {
  // Empty input cursors.
  auto input1 = sample_input_1_.BuildCursor();
  auto input2 = sample_input_2_.BuildCursor();

  auto input1_saved = input1.get();
  auto input2_saved = input2.get();

  std::unique_ptr<const SingleSourceProjector> left_project(
      ProjectAttributeAt(0));
  auto right_project = make_unique<CompoundMultiSourceProjector>();
  right_project->add(0, ProjectAttributeAtAs(1, "col0"));
  right_project->add(1, ProjectAttributeAtAs(0, "col1"));

  auto rowid_merge = CreateRowidMergeJoin(*left_project, *right_project,
      std::move(input1), std::move(input2));

  std::unique_ptr<CursorTransformerWithSimpleHistory> spy_transformer(
      PrintingSpyTransformer());
  rowid_merge->ApplyToChildren(spy_transformer.get());

  ASSERT_EQ(2, spy_transformer->GetHistoryLength());
  EXPECT_EQ(input1_saved, spy_transformer->GetEntryAt(0)->original());
  EXPECT_EQ(input2_saved, spy_transformer->GetEntryAt(1)->original());
}

}  // namespace supersonic
