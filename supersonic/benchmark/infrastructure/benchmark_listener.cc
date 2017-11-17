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
// The implementation of benchmark listener.

#include "supersonic/benchmark/infrastructure/benchmark_listener.h"

#include "supersonic/base/infrastructure/block.h"
#include "supersonic/cursor/base/cursor.h"
#include "supersonic/utils/walltime.h"
#include "supersonic/utils/timer.h"

namespace supersonic {

namespace {

const int64_t kNumNanosInMicro = 1000ll;

// Implementation of spy based performance benchmarking listener.
// Counts time that has been spent during "next" invocations.
class BenchmarkListenerImpl : public BenchmarkListener {
 public:
  BenchmarkListenerImpl()
      : next_calls_(0),
        rows_processed_(0),
        total_time_nanos_(0),
        first_next_time_nanos_(0) {}

  virtual ~BenchmarkListenerImpl() {}

  virtual void BeforeNext(const string& id, rowcount_t max_row_count) {}

  virtual void AfterNext(const string& id,
                         rowcount_t max_row_count,
                         const ResultView& result_view,
                         int64_t time_nanos);

  virtual int64_t NextCalls() const {
    return next_calls_;
  }

  virtual int64_t RowsProcessed() const {
    return rows_processed_;
  }

  virtual int64_t TotalTimeUsec() const {
    return total_time_nanos_ / kNumNanosInMicro;
  }

  virtual int64_t FirstNextTimeUsec() const {
    return first_next_time_nanos_ / kNumNanosInMicro;
  }

  virtual string GetResults() const;

  virtual string GetResults(const BenchmarkListener& subtract) const;

 private:
  int64_t next_calls_;
  int64_t rows_processed_;
  int64_t total_time_nanos_;
  int64_t first_next_time_nanos_;
};

void BenchmarkListenerImpl::AfterNext(const string& id,
                                      rowcount_t max_row_count,
                                      const ResultView& result_view,
                                      int64_t time_nanos) {
  next_calls_++;
  rows_processed_ += result_view.has_data() ?
      result_view.view().row_count() : 0;
  total_time_nanos_ += time_nanos;
  if (next_calls_ == 1) {
    first_next_time_nanos_ = time_nanos;
  }
}

string BenchmarkListenerImpl::GetResults() const {
  return StringPrintf("%" PRIi64 " rows in %" PRIi64" calls in %f ms (rows %f/s)",
                      rows_processed_, next_calls_,
                          TotalTimeUsec() / (kNumMillisPerSecond * 1.0),
                      (rows_processed_* kNumMicrosPerSecond * 1.0) /
                          TotalTimeUsec());
}

string BenchmarkListenerImpl::GetResults(
    const BenchmarkListener& subtract) const {
  return StringPrintf("%" PRId64" rows, %f ms, %f rows/s",
      rows_processed_,
      (TotalTimeUsec() - subtract.TotalTimeUsec()) /
          (kNumMillisPerSecond * 1.0),
      (rows_processed_ * kNumMicrosPerSecond * 1.0) /
          (TotalTimeUsec() - subtract.TotalTimeUsec()));
}

}  // namespace

BenchmarkListener* CreateBenchmarkListener() {
  return new BenchmarkListenerImpl();
}

}  // namespace supersonic
