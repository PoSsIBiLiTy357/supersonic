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
//
// Column-wise Sort implementation. Sorts the input progressively by successive
// sort key columns. First, sorts globally by the first key column. Then,
// identifies row ranges for that have the same value in the first key column,
// and sorts these ranges by the second column. Rinse and repeat, until there's
// no more key columns or no more ranges.
//
// Example 1:
// Input:
// { 5, 3 }, { 4, 2 }, { 1, 2 }, { 4, 5 }, { 3, 1 }, { 3, 3 }, { 4, 1 }
// Sorting by first ASC, second ASC
// After step 1 (sorting by first ASC); example result:
// { 1, 2 }, { 3, 1 }, { 3, 3 }, { 4, 2 }, { 4, 5 }, { 4, 1 }, { 5, 3 }
// Row ranges to sort further by the second column:
// * [1,2] (2 records with value = 3)
// * [3,5] (3 records with value = 4)
// After step 2, i.e. sorting these two ranges by the second column:
// { 1, 2 }, { 3, 1 }, { 3, 3 }, { 4, 1 }, { 4, 2 }, { 4, 5 }, { 5, 3 }
// We're stopping, since there's no more key columns.
//
// Example 2:
// Input:
// { 5, 3 }, { 4, 2 }, { 1, 2 }, { 3, 3 }
// Sorting by first ASC, second ASC
// After step 1 (sorting by first ASC):
// { 1, 2 }, { 3, 3 }, { 4, 2 }, { 5, 3 }
// We're stopping here, as there are no row ranges to sort further by the
// second column (i.e. the first column turned out to be an unique key).
//
// To handle NULLs, we notice that (for the purpose of Sort) NULLs are equal
// to each other, and smaller than anything non-NULL. Hence, we can partition
// the input by moving all NULL rows to the top, and then we can sort the rest
// ignoring nullability.
//
// Computational complexity (in terms of 'atomic' comparison operations):
//
// Let's consider corner cases first (and ignore NULLs for now):
// (1) the first column is an unique key. Processing the first column requires
// sorting (n log2 n) and a linear scan to seek equal values O(n). Overall,
// (n * (log2(n) + 1)) comparisons.
// (2) k columns, all but the last one are constant. The algorithm will process
// all k columns, preserve the single large sort range till the last column.
// So, in every step, we pay for full sort and linear scan; overall,
// (k * n * (log2(n) + 1)) comparisons.
// Now, let's consider an 'in-the-middle' case, with k columns, each with
// m randomly distributed distinct values. At each sort step, the number of
// ranges will increase m times, but the length of a single range will decrease
// m times, too. So, at every step, we pay for m smaller sorts
// (m^k * (n/(m^k) * log2(n/(m^k)))) = (n * log2(n/(m^k)), plus a linear scan
// (n); overall, Sum over k of (n * (log2(n/(m^k)) + 1)). That is, compared
// to the case (2) above, in each step we have a factor of (log2(n/(m^k)) + 1
// instead of log2(n) + 1; i.e. sort times of successive columns are expected
// to decrease linearly.
// All in all, this algorithm performs the same number of comparisons as
// a 'naive' row-based sort, but it has the advantage of inlined comparators,
// and better data locality (if we assume column-based storage).
//
// Considering NULLs now: the partitioning step that separates NULLs from non-
// NULLs costs n comparisons (partitioning is O(n)), but it allows to remove
// NULL checks from n log2(n) comparisons in the further stage of the sort.
// Thus, compared to the naive approach, it reduces the number of comparisons
// from 2*n*log2(n) to n*(log2(n)+1).
//
// Currently, for sorting a single range, the implementation uses STL sort.
// The STL implementation is 'divide-and-conquer', so it can be considered
// somewhat cache-friendly (at some point in the recursion, 'divide' fits
// in cache). It is worth checking if it can be improved upon by using more
// sophisticated sort (e.g. radix sort), but I would not expect spectacular
// results (after all, if there was a faster sort, gcc would likely use it in
// its STL implementation).

#include "supersonic/cursor/core/sort.h"

#include "supersonic/utils/std_namespace.h"
#include "supersonic/utils/basictypes.h"
#include "supersonic/utils/integral_types.h"
#include <glog/logging.h>
#include "supersonic/utils/logging-inl.h"
#include "supersonic/utils/exception/failureor.h"
#include "supersonic/base/exception/exception.h"
#include "supersonic/base/exception/exception_macros.h"
#include "supersonic/base/infrastructure/bit_pointers.h"
#include "supersonic/base/infrastructure/block.h"
#include "supersonic/base/infrastructure/projector.h"
#include "supersonic/base/infrastructure/tuple_schema.h"
#include "supersonic/base/infrastructure/types.h"
#include "supersonic/base/infrastructure/types_infrastructure.h"
#include "supersonic/base/infrastructure/variant_pointer.h"
#include "supersonic/base/memory/memory.h"
#include "supersonic/cursor/base/cursor.h"
#include "supersonic/cursor/proto/cursors.pb.h"
#include "supersonic/cursor/base/operation.h"
#include "supersonic/cursor/core/compute.h"
#include "supersonic/cursor/core/limit.h"
#include "supersonic/cursor/core/merge_union_all.h"
#include "supersonic/cursor/core/ownership_taker.h"
#include "supersonic/cursor/core/project.h"
#include "supersonic/cursor/core/scan_view.h"
#include "supersonic/cursor/infrastructure/basic_cursor.h"
#include "supersonic/cursor/infrastructure/basic_operation.h"
#include "supersonic/cursor/infrastructure/file_io.h"
#include "supersonic/cursor/infrastructure/ordering.h"
#include "supersonic/cursor/infrastructure/table.h"
#include "supersonic/cursor/infrastructure/view_cursor.h"
#include "supersonic/cursor/infrastructure/writer.h"
#include "supersonic/expression/base/expression.h"
#include "supersonic/expression/core/string_expressions.h"
#include "supersonic/expression/core/projecting_bound_expressions.h"
#include "supersonic/expression/core/projecting_expressions.h"
#include "supersonic/expression/infrastructure/expression_utils.h"
#include "supersonic/proto/specification.pb.h"
#include "supersonic/proto/supersonic.pb.h"
#include "supersonic/utils/file.h"
#include "supersonic/utils/file_util.h"
#include "supersonic/utils/pointer_vector.h"
#include "supersonic/utils/stl_util.h"

namespace supersonic {

namespace {

using util::gtl::PointerVector;

template<DataType type, bool descending>
struct LessThanComparator {
  explicit LessThanComparator(
      const typename TypeTraits<type>::cpp_type* const data)
      : data(data) {}
  bool operator()(const int64_t a, const int64_t b) const {
    return (descending ? ThreeWayCompare<type, type, true>(data[b], data[a])
                       : ThreeWayCompare<type, type, true>(data[a], data[b]))
        == RESULT_LESS;
  }
  const typename TypeTraits<type>::cpp_type* data;
};

// Represents a range of rows that need to be sorted further. Initially,
// there's a single range that includes the entire input. As sort progresses,
// the range gets broken into smaller pieces.
struct Range {
  Range() : from(0), to(0) {}
  Range(int64_t from, int64_t to) : from(from), to(to) {}
  int64_t from;
  int64_t to;
};

// Predicate used in stl::partition to percolate all NULLs to the top/bottom.
template<bool descending>
struct NullPartitionPredicate {
  explicit NullPartitionPredicate(bool_const_ptr is_null) : is_null(is_null) {}
  bool operator()(int64_t i) { return is_null[i] != descending; }
  bool_const_ptr is_null;
};

template<DataType type, bool descending>
void SortNonNullRange(const typename TypeTraits<type>::cpp_type* data,
                      const Range& source,
                      vector<Range>* target,
                      Permutation* permutation,
                      bool is_last_column) {
  LessThanComparator<type, descending> less_than(data);
  permutation->Sort(source.from, source.to, less_than);
  if (is_last_column) return;
  int64_t current_from = source.from;
  for (int64_t j = current_from + 1; j < source.to; ++j) {
    if (less_than(permutation->at(current_from), permutation->at(j))) {
      if (j - current_from > 1) {
        target->push_back(Range(current_from, j));
      }
      current_from = j;
    }
  }
  if (source.to - current_from > 1) {
    target->push_back(Range(current_from, source.to));
  }
}

template<DataType type, bool descending, bool is_always_not_null>
void SortRange(const typename TypeTraits<type>::cpp_type* data,
               bool_const_ptr is_null,
               const Range& source,
               vector<Range>* target,
               Permutation* permutation,
               bool is_last_column) {
  if (is_always_not_null) {
    SortNonNullRange<type, descending>(data, source, target, permutation,
                                       is_last_column);
  } else {
    NullPartitionPredicate<descending> predicate(is_null);
    rowcount_t partition = permutation->Partition(source.from, source.to,
                                                  predicate);
    if (partition > 1) {
      Range range(source.from, source.from + partition);
      if (descending) {
        SortNonNullRange<type, true>(data, range, target, permutation,
                                     is_last_column);
      } else {
        // The range now contains NULLs; need to sort further columns.
        if (!is_last_column) target->push_back(range);
      }
    }
    if (source.to - partition > 1) {
      Range range(source.from + partition, source.to);
      if (descending) {
        // The range now contains NULLs; need to sort further columns.
        if (!is_last_column) target->push_back(range);
      } else {
        SortNonNullRange<type, false>(data, range, target, permutation,
                                      is_last_column);
      }
    }
  }
}

template<DataType type, bool descending, bool is_always_not_null>
void SortColumnResolved(
    const typename TypeTraits<type>::cpp_type* data,
    bool_const_ptr is_null,
    const vector<Range>& source,
    vector<Range>* target,
    Permutation* permutation,
    bool is_last_column) {
  for (auto i: source) {
    SortRange<type, descending, is_always_not_null>(data, is_null, i,
                                                    target, permutation,
                                                    is_last_column);
  }
}

template<DataType type>
void SortColumn(bool descending,
                const typename TypeTraits<type>::cpp_type* data,
                bool_const_ptr is_null,
                const vector<Range>& source,
                vector<Range>* target,
                Permutation* permutation,
                bool is_last_column) {
  if (is_null == NULL) {
    if (descending) {
      SortColumnResolved<type, true, true>(data, is_null, source,
                                           target, permutation,
                                           is_last_column);
    } else {
      SortColumnResolved<type, false, true>(data, is_null, source,
                                            target, permutation,
                                            is_last_column);
    }
  } else {
    if (descending) {
      SortColumnResolved<type, true, false>(data, is_null, source,
                                            target, permutation,
                                            is_last_column);
    } else {
      SortColumnResolved<type, false, false>(data, is_null, source,
                                             target, permutation,
                                             is_last_column);
    }
  }
}

struct ColumnSorter {
  template<DataType type>
  void operator()() const {
    typedef typename TypeTraits<type>::cpp_type cpp_type;
    SortColumn<type>(descending, data.as<type>(), is_null,
                     source_ranges, target_ranges, permutation, is_last_column);
  }
  bool descending;
  const VariantConstPointer data;
  bool_const_ptr is_null;
  const vector<Range>& source_ranges;
  vector<Range>* target_ranges;
  Permutation* permutation;
  bool is_last_column;
};

void SortTypedColumn(DataType type,
                     bool descending,
                     const VariantConstPointer data,
                     bool_const_ptr is_null,
                     const vector<Range>& source_ranges,
                     vector<Range>* target_ranges,
                     Permutation* permutation,
                     bool is_last_column) {
  ColumnSorter sorter = {
    descending,
    data,
    is_null,
    source_ranges,
    target_ranges,
    permutation,
    is_last_column,
  };
  TypeSpecialization<void, ColumnSorter>(type, sorter);
}

class BasicMerger : public Merger {
 public:
  BasicMerger(TupleSchema schema, StringPiece temporary_directory_prefix,
              BufferAllocator* allocator)
      : schema_(schema),
        temporary_directory_prefix_(temporary_directory_prefix.ToString()),
        allocator_(allocator) {}

  FailureOrVoid AddSorted(unique_ptr<Cursor> cursor) {
    std::unique_ptr<file::FileRemover> temp_file(new file::FileRemover(
        TempFile::Create(temporary_directory_prefix_.c_str())));
    if (temp_file->get() == NULL) {
      THROW(new Exception(ERROR_TEMP_FILE_CREATION_ERROR,
                          StrCat("Couldn't create temporary file in ",
                                 temporary_directory_prefix_)));
    }
    {
      std::unique_ptr<Sink> file_sink(
          FileOutput(temp_file->get(), DO_NOT_TAKE_OWNERSHIP));
      Writer part_writer(std::move(cursor));
      FailureOr<rowcount_t> write_all_result =
          part_writer.WriteAll(file_sink.get());
      if (write_all_result.is_failure() &&
          write_all_result.exception().return_code() == WAITING_ON_BARRIER) {
        THROW(new Exception(ERROR_NOT_IMPLEMENTED,
                            "BasicMerger doesn't handle WAITING_ON_BARRIER."));
      }
      // Make sure to finalize the file sink before propagating write_all_result
      FailureOrVoid file_sink_finalize_result = file_sink->Finalize();
      PROPAGATE_ON_FAILURE(write_all_result);
      PROPAGATE_ON_FAILURE(file_sink_finalize_result);
    }
    // TODO(user): Don't just ignore the util::Status object!
    // We didn't opensource util::task::Status.
    temp_file->get()->Seek(0);
    file_buffers_.emplace_back(temp_file.release());
    return Success();
  }

  // TODO(user): Consider some pre-merging phase if the number of files is big
  // enough.
  FailureOrOwned<Cursor> Merge(unique_ptr<const BoundSortOrder> sort_order,
                               unique_ptr<Cursor> additional) {
    vector<unique_ptr<Cursor>> merged_cursors;
    while (!file_buffers_.empty()) {
      FailureOrOwned<Cursor> file_cursor(
          FileInput(schema_,
                    file_buffers_.back()->release(),
                    true,  // delete_when_done
                    allocator_));
      file_buffers_.pop_back();
      PROPAGATE_ON_FAILURE(file_cursor);
      merged_cursors.push_back(file_cursor.move());
    }
    if ((bool) additional) {
      // Use the additional cursor as the last source.
      merged_cursors.emplace_back(std::move(additional));
    }
    FailureOrOwned<Cursor> merged(
        BoundMergeUnionAll(std::move(sort_order),
                           std::move(merged_cursors),
                           allocator_));
    PROPAGATE_ON_FAILURE(merged);
    return merged;
  }

  virtual bool empty() const {
    return file_buffers_.empty();
  }

 private:
  TupleSchema schema_;
  string temporary_directory_prefix_;
  BufferAllocator* allocator_;
  PointerVector<file::FileRemover> file_buffers_;
  DISALLOW_COPY_AND_ASSIGN(BasicMerger);
};

class UnbufferedSorter : public Sorter {
 public:
  // Takes ownership of sort_order. allocator should be valid as long as
  // UnbufferedSorter exists and then as long as the cursor returned from
  // GetResultCursor() exists.
  UnbufferedSorter(const TupleSchema& schema,
                   unique_ptr<const BoundSortOrder> sort_order,
                   StringPiece temporary_directory_prefix,
                   BufferAllocator* allocator)
      : sort_order_(std::move(sort_order)),
        allocator_(allocator),
        merger_(CreateMerger(schema, temporary_directory_prefix, allocator)) {}

  virtual ~UnbufferedSorter() {}

  virtual FailureOr<rowcount_t> Write(const View& data) {
    rowcount_t row_count = data.row_count();
    FailureOrOwned<Cursor> sorted = SortView(data);
    PROPAGATE_ON_FAILURE(sorted);
    PROPAGATE_ON_FAILURE(merger_->AddSorted(sorted.move()));
    return Success(row_count);
  }

  FailureOrOwned<Cursor> GetResultCursor() {
    FailureOrOwned<Cursor> merged = merger_->Merge(std::move(sort_order_), nullptr);
    PROPAGATE_ON_FAILURE(merged);
    return Success(merged.move());
  }

  // Return all the written data sorted and merged with an optional
  // sorted_cursor.
  FailureOrOwned<Cursor> GetResultCursorMergedWith(unique_ptr<Cursor> sorted_cursor) {
    if (merger_->empty() && sorted_cursor != NULL) {
      return Success(std::move(sorted_cursor));
    } else {
      FailureOrOwned<Cursor> merged =
          merger_->Merge(std::move(sort_order_), std::move(sorted_cursor));
      PROPAGATE_ON_FAILURE(merged);
      return Success(merged.move());
    }
  }

  // Returns a Cursor containing sorted data from the input view. View should be
  // valid as long as the Cursor exists.
  FailureOrOwned<Cursor> SortView(const View& view) {
    auto permutation = make_unique<Permutation>(view.row_count());
    SortPermutation(*sort_order_, view, permutation.get());
    FailureOrOwned<Cursor> sorted = BoundScanViewWithSelection(
        view, permutation->size(), permutation->permutation(),
        allocator_, Cursor::kDefaultRowCount);
    PROPAGATE_ON_FAILURE(sorted);
    return Success(TakeOwnership(sorted.move(), std::move(permutation)));
  }

 private:
  unique_ptr<const BoundSortOrder> sort_order_;
  BufferAllocator* allocator_;
  unique_ptr<Merger> merger_;
  DISALLOW_COPY_AND_ASSIGN(UnbufferedSorter);
};

class BufferingSorter : public Sorter {
 public:
  // Takes ownership of sort_order. allocator should be valid as long as
  // BufferingSorter exists and then as long as the cursor returned from
  // GetResultCursor() exists.
  BufferingSorter(const TupleSchema& schema,
                  unique_ptr<const BoundSortOrder> sort_order,
                  size_t memory_quota,
                  StringPiece temporary_directory_prefix,
                  BufferAllocator* allocator)
      : allocator_(allocator),
        softquota_bypass_allocator_(
            new SoftQuotaBypassingBufferAllocator(allocator_,
                                                  memory_quota / 4)),
        // Current implementation can exceed the allocator's soft quota by a
        // factor of 2. This is because the implementation uses Table as a
        // buffer, which doubles its block until it exceeds the soft quota. For
        // safety we halve the supplied quota value.
        materialization_allocator_(
            new MemoryLimit(memory_quota / 2, false,
                            softquota_bypass_allocator_.get())),
        memory_buffer_(
            new Table(schema, materialization_allocator_.get())),
        unbuffered_sorter_(schema, std::move(sort_order), temporary_directory_prefix,
                           allocator) {}

  virtual ~BufferingSorter() {}

  virtual FailureOr<rowcount_t> Write(const View& data) {
    {
      TableSink table_sink(memory_buffer_.get());
      FailureOr<rowcount_t> written = table_sink.Write(data);
      PROPAGATE_ON_FAILURE(written);
      if (written.get() > 0) {
        return written;
      }
    }
    // Didn't manage to write anything to memory_buffer_. Flush memory_buffer_
    // and try writing again.
    PROPAGATE_ON_FAILURE(Flush());
    CHECK_EQ(0, memory_buffer_->row_count());
    {
      TableSink table_sink(memory_buffer_.get());
      FailureOr<rowcount_t> written = table_sink.Write(data);
      PROPAGATE_ON_FAILURE(written);
      if (written.get() > 0) {
        return written;
      }
    }
    THROW(new Exception(
        ERROR_MEMORY_EXCEEDED,
        StrCat("Couldn't copy any data to an empty Table in BufferingSorter::",
               "Write. Probably hard quota ran out. ",
               "materialization_allocator_: ",
               "quota=", materialization_allocator_->GetQuota(),
               ", available=", materialization_allocator_->Available(),
               ", usage=", materialization_allocator_->GetUsage(),
               "; allocator_ (parent): available=", allocator_->Available(),
               " (allocator_->Available() can be smaller than "
               " materialization_allocator_->Available() because of "
               "SoftQuotaBypassingBufferAllocator)")));
  }

  FailureOrOwned<Cursor> GetResultCursor() {
    // No need to flush current contents of memory_buffer_.
    FailureOrOwned<Cursor> last_sorted =
        unbuffered_sorter_.SortView(memory_buffer_->view());
    PROPAGATE_ON_FAILURE(last_sorted);
    auto last_sorted_owning = TakeOwnership(
        last_sorted.move(), std::move(softquota_bypass_allocator_),
        std::move(materialization_allocator_), std::move(memory_buffer_));
    return unbuffered_sorter_.GetResultCursorMergedWith(
        std::move(last_sorted_owning));
  }

 private:
  // Flush the current memory_buffer_ to unbuffered_sorter_.
  FailureOrVoid Flush() {
    if (memory_buffer_->row_count() > 0) {
      FailureOr<rowcount_t> written =
          unbuffered_sorter_.Write(memory_buffer_->view());
      PROPAGATE_ON_FAILURE(written);
      CHECK_EQ(written.get(), memory_buffer_->row_count());
      memory_buffer_->Clear();
      if (materialization_allocator_->Available() == 0) {
        memory_buffer_->Compact();
      }
    }
    return Success();
  }

  BufferAllocator* allocator_;

  // This allocator is for "bypassing" a certain amount of potential soft quota
  // in allocator_, so Sort will be able to grow its internal Table considerably
  // even if there's no soft quota left. This should prevent big performance
  // degradation is such cases.
  std::unique_ptr<BufferAllocator> softquota_bypass_allocator_;

  // materialization_allocator_ is MemoryLimit with soft quota.
  std::unique_ptr<MemoryLimit> materialization_allocator_;
  std::unique_ptr<Table> memory_buffer_;
  UnbufferedSorter unbuffered_sorter_;
  DISALLOW_COPY_AND_ASSIGN(BufferingSorter);
};

class SortCursor : public BasicCursor {
 public:
  SortCursor(unique_ptr<const BoundSortOrder> sort_order,
             unique_ptr<const BoundSingleSourceProjector> result_projector,
             size_t memory_quota,
             StringPiece temporary_directory_prefix,
             BufferAllocator* allocator,
             unique_ptr<Cursor> child)
      : BasicCursor(result_projector->result_schema()),
        is_waiting_on_barrier_supported_(child->IsWaitingOnBarrierSupported()),
        writer_(std::move(child)),
        result_projector_(std::move(result_projector)),
        sorter_(CreateBufferingSorter(writer_.schema(), std::move(sort_order),
                                      memory_quota, temporary_directory_prefix,
                                      allocator)),
        sorter_sink_(sorter_.get()) {}

  virtual ResultView Next(rowcount_t max_row_count) {
    if (result_.get() == NULL) {
      PROPAGATE_ON_FAILURE(ProcessData());
      if (result_.get() == NULL) {
        // No failure, but hasn't completed.
        CHECK(writer_.is_waiting_on_barrier());
        return ResultView::WaitingOnBarrier();
      }
    }
    return result_->Next(max_row_count);
  }

  virtual bool IsWaitingOnBarrierSupported() const {
    return is_waiting_on_barrier_supported_;
  }

  virtual void Interrupt() {
    writer_.Interrupt();
    // There is a race between checking result_ for NULL and result_.reset(...)
    // in ProcessData.
    if (result_ != NULL) result_->Interrupt();
  }

  virtual void ApplyToChildren(CursorTransformer* transformer) {
    writer_.ApplyToIterator(transformer);
  }

  virtual CursorId GetCursorId() const { return SORT; }

 private:
  FailureOrVoid ProcessData();

  void SetResultWithProjection(unique_ptr<Cursor> result) {
    result_ = BoundProject(std::move(result_projector_), std::move(result));
  }

  bool is_waiting_on_barrier_supported_;
  Writer writer_;
  unique_ptr<const BoundSingleSourceProjector> result_projector_;
  unique_ptr<Cursor> result_;
  unique_ptr<Sorter> sorter_;
  SorterSink sorter_sink_;
  DISALLOW_COPY_AND_ASSIGN(SortCursor);
};

// TODO(user): add support streaming partial sort (i.e. sort within record).
FailureOrVoid SortCursor::ProcessData() {
  while (!writer_.is_eos()) {
    FailureOr<rowcount_t> outcome = writer_.WriteAll(&sorter_sink_);
    PROPAGATE_ON_FAILURE(outcome);
    if (writer_.is_waiting_on_barrier()) {
      // Better luck next time. (And, continue from where we started).
      return Success();
    }
  }
  PROPAGATE_ON_FAILURE(sorter_sink_.Finalize());
  FailureOrOwned<Cursor> sorter_result = sorter_->GetResultCursor();
  PROPAGATE_ON_FAILURE(sorter_result);
  SetResultWithProjection(sorter_result.move());
  return Success();
}

class SortOperation : public BasicOperation {
 public:
  // Takes ownership of the sort order and the projector.
  SortOperation(unique_ptr<const SortOrder> sort_order,
                unique_ptr<const SingleSourceProjector> result_projector,
                size_t memory_quota,
                StringPiece temporary_directory_prefix,
                unique_ptr<Operation> child)
      : BasicOperation(std::move(child)),
        sort_order_(std::move(sort_order)),
        result_projector_(std::move(result_projector)),
        memory_quota_(memory_quota),
        temporary_directory_prefix_(temporary_directory_prefix.ToString()) {
    CHECK_NOTNULL(sort_order_.get());
  }

  virtual ~SortOperation() {}

  virtual FailureOrOwned<Cursor> CreateCursor() const {
    FailureOrOwned<Cursor> child_cursor = child()->CreateCursor();
    PROPAGATE_ON_FAILURE(child_cursor);
    const TupleSchema& schema = child_cursor->schema();
    FailureOrOwned<const BoundSortOrder> sort_order(sort_order_->Bind(schema));
    PROPAGATE_ON_FAILURE(sort_order);
    std::unique_ptr<const BoundSingleSourceProjector> result_projector_ptr;
    if (result_projector_.get() != NULL) {
      FailureOrOwned<const BoundSingleSourceProjector> result_projector(
          result_projector_->Bind(schema));
      PROPAGATE_ON_FAILURE(result_projector);
      result_projector_ptr = result_projector.move();
    }
    // result_projector_ptr can contain NULL. BoundSort handles this.
    return BoundSort(sort_order.move(),
                     std::move(result_projector_ptr),
                     memory_quota_,
                     temporary_directory_prefix_,
                     buffer_allocator(),
                     child_cursor.move());
  }

 private:
  std::unique_ptr<const SortOrder> sort_order_;
  // result_projector_ may be NULL.
  std::unique_ptr<const SingleSourceProjector> result_projector_;
  size_t memory_quota_;
  string temporary_directory_prefix_;
  DISALLOW_COPY_AND_ASSIGN(SortOperation);
};

class ExtendedSortOperation : public BasicOperation {
 public:
  // Takes ownership of the sort order and the projector.
  ExtendedSortOperation(const ExtendedSortSpecification* sort_order,
                        const SingleSourceProjector* result_projector,
                        size_t memory_quota,
                        StringPiece temporary_directory_prefix,
                        unique_ptr<Operation> child)
      : BasicOperation(std::move(child)),
        sort_order_(sort_order),
        result_projector_(result_projector),
        memory_quota_(memory_quota),
        temporary_directory_prefix_(temporary_directory_prefix.ToString()) {
    CHECK_NOTNULL(sort_order);
  }

  virtual ~ExtendedSortOperation() {}

  virtual FailureOrOwned<Cursor> CreateCursor() const {
    FailureOrOwned<Cursor> child_cursor = child()->CreateCursor();
    PROPAGATE_ON_FAILURE(child_cursor);

    std::unique_ptr<const BoundSingleSourceProjector> bound_result_projector;
    if (result_projector_.get() != NULL) {
      FailureOrOwned<const BoundSingleSourceProjector> result_projector(
          result_projector_->Bind(child_cursor->schema()));
      PROPAGATE_ON_FAILURE(result_projector);
      bound_result_projector = result_projector.move();
    }

    // result_projector_ptr can contain NULL. BoundSort handles this.
    return BoundExtendedSort(new ExtendedSortSpecification(*sort_order_),
                             bound_result_projector.release(),
                             memory_quota_,
                             temporary_directory_prefix_,
                             buffer_allocator(),
                             Cursor::kDefaultRowCount,
                             child_cursor.move());
  }

 private:
  std::unique_ptr<const ExtendedSortSpecification> sort_order_;
  // result_projector_ may be NULL.
  std::unique_ptr<const SingleSourceProjector> result_projector_;
  size_t memory_quota_;
  string temporary_directory_prefix_;
  DISALLOW_COPY_AND_ASSIGN(ExtendedSortOperation);
};

}  // namespace

unique_ptr<Merger> CreateMerger(TupleSchema schema,
                     StringPiece temporary_directory_prefix,
                     BufferAllocator* allocator) {
  return make_unique<BasicMerger>(schema, temporary_directory_prefix, allocator);
}

unique_ptr<Sorter> CreateUnbufferedSorter(const TupleSchema& schema,
                                          unique_ptr<const BoundSortOrder> sort_order,
                                          StringPiece temporary_directory_prefix,
                                          BufferAllocator* allocator) {
  return make_unique<UnbufferedSorter>(schema, std::move(sort_order),
                                       temporary_directory_prefix, allocator);
}

unique_ptr<Sorter> CreateBufferingSorter(
    const TupleSchema& schema,
    unique_ptr<const BoundSortOrder> sort_order,
    size_t memory_quota,
    StringPiece temporary_directory_prefix,
    BufferAllocator* allocator) {
  return make_unique<BufferingSorter>(schema, std::move(sort_order),
                                      memory_quota, temporary_directory_prefix,
                                      allocator);
}

void SortPermutation(const BoundSortOrder& sort_order,
                     const View& input,
                     Permutation* permutation) {
  CHECK_EQ(input.row_count(), permutation->size());
  // Pair for double buffering.
  pair<vector<Range>, vector<Range> > ranges;
  vector<Range>* source_ranges = &ranges.first;
  vector<Range>* target_ranges = &ranges.second;
  source_ranges->push_back(Range(0, input.row_count()));
  int num_columns = sort_order.schema().attribute_count();
  for (int i = 0; i < num_columns; ++i) {
    const Attribute attribute = sort_order.schema().attribute(i);
    const Column& input_column = input.column(
        sort_order.source_attribute_position(i));
    SortTypedColumn(attribute.type(),
                    sort_order.column_order(i) == DESCENDING,
                    input_column.data(), input_column.is_null(),
                    *source_ranges, target_ranges,
                    permutation,
                    i == num_columns - 1);
    if (target_ranges->empty()) break;
    std::swap(source_ranges, target_ranges);
    target_ranges->clear();
  }
}

unique_ptr<Operation> Sort(
    unique_ptr<const SortOrder> sort_order,
    unique_ptr<const SingleSourceProjector> result_projector,
    size_t memory_quota,
    unique_ptr<Operation> child) {
  return make_unique<SortOperation>(
      std::move(sort_order), std::move(result_projector),
      memory_quota, "", std::move(child));
}

unique_ptr<Operation> ExtendedSort(const ExtendedSortSpecification* specification,
                                   const SingleSourceProjector* result_projector,
                                   size_t memory_limit,
                                   unique_ptr<Operation> child) {
  return make_unique<ExtendedSortOperation>(specification, result_projector,
                                            memory_limit, "", std::move(child));
}

unique_ptr<Operation> SortWithTempDirPrefix(
    unique_ptr<const SortOrder> sort_order,
    unique_ptr<const SingleSourceProjector> result_projector,
    size_t memory_quota,
    StringPiece temporary_directory_prefix,
    unique_ptr<Operation> child) {
  return make_unique<SortOperation>(
      std::move(sort_order), std::move(result_projector),
      memory_quota, temporary_directory_prefix, std::move(child));
}

FailureOrOwned<Cursor> BoundSort(
    unique_ptr<const BoundSortOrder> sort_order,
    unique_ptr<const BoundSingleSourceProjector> result_projector,
    size_t memory_quota,
    StringPiece temporary_directory_prefix,
    BufferAllocator* allocator,
    unique_ptr<Cursor> child) {
  if (result_projector == nullptr) {
    auto all = ProjectAllAttributes();
    result_projector = SucceedOrDie(all->Bind(child->schema()));
  }

  return Success(make_unique<SortCursor>(
      std::move(sort_order), std::move(result_projector),
      memory_quota,
      temporary_directory_prefix,
      allocator,
      std::move(child)));
}

// This methods works by creating an additional attribute for each key attribute
// that is case insensitive - which contains the attributed casted uppercase.
// Then, it proceeds to sort the computed cursor using the regular BoundSort
// (using the uppercase versions of the case insensitive key attributes).
// Finally, if a limit argument is supplied to sort specification, it wraps
// the cursor after sort with BoundLimit.
FailureOrOwned<Cursor> BoundExtendedSort(
    const ExtendedSortSpecification* sort_specification,
    const BoundSingleSourceProjector* result_projector,
    size_t memory_quota,
    StringPiece temporary_directory_prefix,
    BufferAllocator* allocator,
    rowcount_t max_row_count,
    unique_ptr<Cursor> child) {
  std::unique_ptr<const ExtendedSortSpecification> owned_sort_specification(
      sort_specification);
  std::unique_ptr<const BoundSingleSourceProjector> owned_result_projector(
      result_projector);

  // First we check that sort order does not have duplicate key(s).
  set<string> case_insensitive_keys_field_paths;
  set<string> case_sensitive_keys_field_paths;
  for (size_t i = 0; i < sort_specification->keys_size(); ++i) {
    if (!sort_specification->keys(i).case_sensitive() &&
        child->schema().LookupAttribute(
            sort_specification->keys(i).attribute_name()).type() == STRING) {
      if (case_insensitive_keys_field_paths.find(
          sort_specification->keys(i).attribute_name()) !=
          case_insensitive_keys_field_paths.end()) {
        THROW(new Exception(
              ERROR_INVALID_ARGUMENT_VALUE,
              StrCat("Duplicate case insensitive key: ",
                     sort_specification->keys(i).attribute_name(),
                     " column in schema (",
                     child->schema().GetHumanReadableSpecification(),
                     ")")));
      }
      case_insensitive_keys_field_paths.insert(
          sort_specification->keys(i).attribute_name());
    } else {
      if (case_sensitive_keys_field_paths.find(
          sort_specification->keys(i).attribute_name()) !=
          case_sensitive_keys_field_paths.end()) {
        THROW(new Exception(
              ERROR_INVALID_ARGUMENT_VALUE,
              StrCat("Duplicate case sensitive key: ",
                     sort_specification->keys(i).attribute_name(),
                     " column in schema (",
                     child->schema().GetHumanReadableSpecification(),
                     ")")));
      }
      case_sensitive_keys_field_paths.insert(
          sort_specification->keys(i).attribute_name());
    }
  }

  // TODO(user): Sort should be able to support case insensitive searches
  // more effectively than casting the entire row into uppercase like the
  // implementation below. Fix this.

  // We have to project out the temporary columns we're going to make.
  size_t initial_number_of_attributes =
      child->schema().attribute_count();

  // We have to project some of the fields to uppercase. We also need to
  // assign unique names to them.
  const string kBaseTemporaryAttributeName = "tmp_uppercase_version_of_";

  // We create the parameters to create a bound compute expression.
  map<string, size_t> uppercase_version_position;
  auto compute_argument = make_unique<ExpressionList>();
  for (size_t i = 0; i < child->schema().attribute_count(); ++i) {
    compute_argument->add(AttributeAt(i));
  }
  set<string> uppercase_version_attribute_names;
  for (size_t i = 0; i < sort_specification->keys_size(); ++i) {
    if (!sort_specification->keys(i).case_sensitive() &&
        child->schema().LookupAttribute(
            sort_specification->keys(i).attribute_name()).type() == STRING) {
      string attribute_name = sort_specification->keys(i).attribute_name();
      if (uppercase_version_position.find(attribute_name) ==
          uppercase_version_position.end()) {
        // Find the name for this attribute;
        string temporary_attribute_name =
            CreateUniqueName(child->schema(),
                             uppercase_version_attribute_names,
                             StrCat(kBaseTemporaryAttributeName,
                                    attribute_name));
        uppercase_version_attribute_names.insert(temporary_attribute_name);
        uppercase_version_position[attribute_name] = compute_argument->size();
        compute_argument->add(Alias(
            temporary_attribute_name, ToUpper(NamedAttribute(attribute_name))));
      }
    }
  }

  FailureOrOwned<BoundExpressionList> bound_compute_argument(
      compute_argument->DoBind(child->schema(),
                               allocator,
                               max_row_count));
  PROPAGATE_ON_FAILURE(bound_compute_argument);
  FailureOrOwned<BoundExpression> compound_expression =
      BoundCompoundExpression(bound_compute_argument.move());
  PROPAGATE_ON_FAILURE(compound_expression);
  FailureOrOwned<BoundExpressionTree> compound_expression_tree =
      CreateBoundExpressionTree(compound_expression.move(),
                                allocator,
                                max_row_count);
  PROPAGATE_ON_FAILURE(compound_expression_tree);
  FailureOrOwned<Cursor> bound_compute = BoundCompute(
      compound_expression_tree.move(),
      allocator,
      max_row_count,
      std::move(child));
  PROPAGATE_ON_FAILURE(bound_compute);
  child = bound_compute.move();

  // We create BoundSortOrder for BoundSort.
  auto keys_projector = make_unique<BoundSingleSourceProjector>(child->schema());
  vector<ColumnOrder> keys_orders;

  for (size_t i = 0; i < sort_specification->keys_size(); ++i) {
    keys_orders.push_back(ColumnOrder(
        sort_specification->keys(i).column_order()));
    if (sort_specification->keys(i).case_sensitive() ||
        child->schema().LookupAttribute(
            sort_specification->keys(i).attribute_name()).type() != STRING) {
      keys_projector->Add(child->schema().LookupAttributePosition(
          sort_specification->keys(i).attribute_name()));
    } else {
      keys_projector->Add(uppercase_version_position[
          sort_specification->keys(i).attribute_name()]);
    }
  }

  // We also need to project out the temporary attributes.
  if (owned_result_projector.get() == NULL) {
    auto output_projector = make_unique<BoundSingleSourceProjector>(child->schema());
    for (size_t i = 0; i < initial_number_of_attributes; ++i) {
      output_projector->Add(i);
    }
    owned_result_projector = std::move(output_projector);
  }

  FailureOrOwned<Cursor> freshly_sorted_cursor =
      BoundSort(make_unique<BoundSortOrder>(std::move(keys_projector), keys_orders),
          std::move(owned_result_projector),
          memory_quota,
          temporary_directory_prefix,
          allocator,
          std::move(child));

  PROPAGATE_ON_FAILURE(freshly_sorted_cursor);

  auto final_cursor = freshly_sorted_cursor.move();

  // TODO(user): Sort should be able to use this more efficiently then simply
  // layering a limit over a sort. Fix this.
  if (sort_specification->has_limit()) {
    final_cursor = BoundLimit(0, sort_specification->limit(), std::move(final_cursor));
  }

  return Success(std::move(final_cursor));
}

}  // namespace supersonic
