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
// Implementation of benchmark manager.

#include "supersonic/benchmark/manager/benchmark_manager.h"

#include "supersonic/benchmark/infrastructure/benchmark_listener.h"
#include "supersonic/benchmark/infrastructure/cursor_statistics.h"
#include "supersonic/benchmark/infrastructure/node.h"
#include "supersonic/benchmark/infrastructure/tree_builder.h"
#include "supersonic/benchmark/dot/dot_drawer.h"
#include "supersonic/cursor/base/cursor.h"

namespace supersonic {

namespace {

// Creates a DOT output writer depending on the visualisation options.
// The output string will be used by the writer to store the outcome of
// the operation. The caller takes ownership of the writer.
unique_ptr<DOTOutputWriter> CreateWriter(
    GraphVisualisationOptions options, string* output) {
  switch (options.destination) {
    case DOT_FILE:
      return CreateFileOutputWriter(options.file_name);
    case DOT_STRING:
      return CreateStringOutputWriter(output);
    default:
      return nullptr;
  }
}

// Gathers node's data and creates the result graph using drawer. Does not take
// ownership of any of the arguments.
inline void GatherDataAndDraw(BenchmarkTreeNode* node, DOTDrawer* drawer) {
  node->GatherAllData();
  drawer->DrawDOT(*node);
}

}  // namespace

unique_ptr<BenchmarkDataWrapper> SetUpBenchmarkForCursor(unique_ptr<Cursor> cursor) {
  auto tree_builder = make_unique<BenchmarkTreeBuilder>();
  auto result = tree_builder->CreateTree(std::move(cursor));

  return make_unique<BenchmarkDataWrapper>(
      result->move_cursor(),
      std::move(tree_builder),
      result->move_node());
}

string CreateGraph(
    const string& benchmark_name,
    BenchmarkTreeNode* node,
    GraphVisualisationOptions options) {
  string graph_result;

  auto drawer = make_unique<DOTDrawer>(
    CreateWriter(options, &graph_result), benchmark_name);

  GatherDataAndDraw(node, drawer.get());
  return graph_result;
}

string PerformBenchmark(
    const string& benchmark_name,
    unique_ptr<Cursor> cursor,
    rowcount_t max_block_size,
    GraphVisualisationOptions options) {
  auto data_wrapper = SetUpBenchmarkForCursor(std::move(cursor));

  auto spied_cursor = data_wrapper->move_cursor();

  while (spied_cursor->Next(max_block_size).has_data()) {}

  return CreateGraph(benchmark_name, data_wrapper->node(), options);
}

}  // namespace supersonic
