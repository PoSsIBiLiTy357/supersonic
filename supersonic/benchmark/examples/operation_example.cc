// Copyright 2012 Google Inc.  All Rights Reserved
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
// Author: tomasz.kaftal@gmail.com (Tomasz Kaftal)
//
// File containing several examples of operation benchmarks.


#include "supersonic/utils/std_namespace.h"
#include "supersonic/benchmark/examples/common_utils.h"
#include "supersonic/cursor/core/merge_union_all.h"
#include "supersonic/supersonic.h"
#include "supersonic/testing/block_builder.h"

#include "supersonic/utils/file_util.h"

#include "supersonic/utils/container_literal.h"
#include "supersonic/utils/random.h"

#include <gflags/gflags.h>
DEFINE_string(output_directory, "", "Directory to which the output files will"
    " be written.");


namespace supersonic {

namespace {

using util::gtl::Container;

const size_t kInputRowCount = 1000000;
const size_t kGroupNum = 50;

unique_ptr<Operation> CreateGroup() {
  MTRandom random(0);
  BlockBuilder<STRING, INT32> builder;
  for (int64_t i = 0; i < kInputRowCount; ++i) {
    builder.AddRow(StringPrintf("test_string_%" PRIi64, i % kGroupNum),
                   random.Rand32());
  }

  auto agg = make_unique<AggregationSpecification>();
  agg->AddAggregation(MAX, "col1", "col1_maxes");

  return GroupAggregate(ProjectAttributeAt(0), std::move(agg),
      nullptr, make_unique<Table>(builder.Build()));
}

unique_ptr<Operation> CreateCompute() {
  MTRandom random(0);
  BlockBuilder<INT32, INT64, DOUBLE> builder;
  for (int64_t i = 0; i < kInputRowCount; ++i) {
    builder.AddRow(random.Rand32(), random.Rand64(), random.RandDouble());
  }

  return Compute(Multiply(AttributeAt(0),
                          Plus(Sin(AttributeAt(2)),
                               Exp(AttributeAt(1)))),
                 make_unique<Table>(builder.Build()));
}

unique_ptr<SortOrder> CreateExampleSortOrder() {
  auto order = make_unique<SortOrder>();
  order->add(ProjectAttributeAt(0), ASCENDING)
       ->add(ProjectAttributeAt(1), DESCENDING);
  return order;
}

unique_ptr<Operation> CreateSort(size_t input_row_count) {
  MTRandom random(0);
  BlockBuilder<INT32, STRING> builder;
  for (int64_t i = 0; i < input_row_count; ++i) {
    builder.AddRow(random.Rand32(), StringPrintf("test_string_%" PRIi64, i));
  }

  return Sort(
      CreateExampleSortOrder(),
      NULL,
      std::numeric_limits<size_t>::max(),
      make_unique<Table>(builder.Build()));
}

unique_ptr<Operation> CreateMergeUnion() {
  vector<unique_ptr<Operation>> input;
  input.emplace_back(CreateSort(kInputRowCount));
  input.emplace_back(CreateSort(2 * kInputRowCount));

  return MergeUnionAll(CreateExampleSortOrder(), std::move(input));
}

unique_ptr<Operation> CreateHashJoin() {
  unique_ptr<Operation> lhs(CreateSort(kInputRowCount));
  unique_ptr<Operation> rhs(CreateGroup());

  auto projector = make_unique<CompoundMultiSourceProjector>();
  projector->add(0, ProjectAllAttributes("L."));
  projector->add(1, ProjectAllAttributes("R."));

  return make_unique<HashJoinOperation>(LEFT_OUTER,
                               ProjectAttributeAt(1),
                               ProjectAttributeAt(0),
                               std::move(projector),
                               UNIQUE,
                               std::move(lhs),
                               std::move(rhs));
}

unique_ptr<Operation> SimpleTreeExample() {
  MTRandom random(0);
  // col0, col1  , col2  , col3, col4
  // name, salary, intern, age , boss_name
  BlockBuilder<STRING, INT32, BOOL, INT32, STRING> builder;
  for (int64_t i = 0; i < kInputRowCount; ++i) {
    builder.AddRow(StringPrintf("Name%" PRIi64, i),
                   (random.Rand16() % 80) * 100,
                   random.Rand16() % 1000 == 0 && i > kGroupNum,
                   (random.Rand16() % 60) + 20,
                   StringPrintf("Name%" PRIi64, i % kGroupNum));
  }

  auto named_columns = Project(
      ProjectRename(Container("name", "salary", "intern", "age", "boss_name"),
                    ProjectAllAttributes()),
      make_unique<Table>(builder.Build()));

  auto filter1 = Filter(NamedAttribute("intern"), ProjectAllAttributes(),
                        std::move(named_columns));

  auto compound = make_unique<CompoundExpression>();
  compound
      ->Add(NamedAttribute("name"))
      ->Add(NamedAttribute("intern"))
      ->Add(NamedAttribute("boss_name"))
      ->AddAs("ratio",
              Divide(NamedAttribute("salary"),
                     NamedAttribute("age")));

  auto compute = Compute(std::move(compound), std::move(filter1));

  auto agg = make_unique<AggregationSpecification>();
  agg->AddAggregation(MAX, "ratio", "max_ratio");

  auto group = GroupAggregate(
      ProjectNamedAttribute("boss_name"),
      std::move(agg),
      nullptr, std::move(compute));

  // Let every fourth pass.
  auto filter2 =
      Filter(Equal(ConstInt32(0), Modulus(Sequence(), ConstInt32(4))),
             ProjectAllAttributes(), make_unique<Table>(builder.Build()));

  auto projector = make_unique<CompoundMultiSourceProjector>();
  projector->add(0, ProjectAllAttributes("L."));
  projector->add(1, ProjectAllAttributes("R."));

  return make_unique<HashJoinOperation>(INNER,
                               ProjectAttributeAt(0),
                               ProjectNamedAttribute("boss_name"),
                               std::move(projector),
                               UNIQUE,
                               std::move(filter2),
                               std::move(group));
}

typedef vector<Operation*>::iterator operation_iterator;

void Run() {
  vector<unique_ptr<Operation>> operations;
  operations.emplace_back(CreateGroup());
  operations.emplace_back(CreateSort(kInputRowCount));
  operations.emplace_back(CreateCompute());
  operations.emplace_back(CreateMergeUnion());
  operations.emplace_back(CreateHashJoin());
  operations.emplace_back(SimpleTreeExample());

  GraphVisualisationOptions options(DOT_FILE);

  for (int i=0; i < operations.size(); i++) {
    options.file_name = File::JoinPath(
        FLAGS_output_directory, StrCat("benchmark_", i, ".dot"));

    // Automatically disposes of the operations after having drawn DOT graphs.
    BenchmarkOperation(
        std::move(operations[i]),
        "Operation Benchmark",
        options,
        /* 16KB (optimised for cache size) */ 16 * Cursor::kDefaultRowCount,
        /* log result? */ false);
  }
}

}  // namespace

}  // namespace supersonic

int main(int argc, char *argv[]) {
  supersonic::SupersonicInit(&argc, &argv);
  supersonic::Run();
}
