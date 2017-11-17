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
// Author: onufry@google.com (Onufry Wojtaszczyk)
//
// Expressions on DATE and DATETIME expressions, bound versions.

#include "supersonic/expression/core/date_bound_expressions.h"

#include <cstddef>
#include <algorithm>
#include "supersonic/utils/std_namespace.h"

#include "supersonic/utils/integral_types.h"
#include <glog/logging.h>
#include "supersonic/utils/logging-inl.h"
#include "supersonic/utils/macros.h"
#include "supersonic/utils/exception/failureor.h"
#include "supersonic/base/exception/exception.h"
#include "supersonic/base/exception/exception_macros.h"
#include "supersonic/base/infrastructure/bit_pointers.h"
#include "supersonic/base/infrastructure/block.h"
#include "supersonic/expression/base/expression.h"
#include "supersonic/expression/core/date_evaluators.h"  // IWYU pragma: keep
#include "supersonic/expression/infrastructure/basic_bound_expression.h"
#include "supersonic/expression/infrastructure/expression_utils.h"
#include "supersonic/expression/proto/operators.pb.h"
#include "supersonic/expression/templated/bound_expression_factory.h"
#include "supersonic/expression/templated/cast_bound_expression.h"
#include "supersonic/proto/supersonic.pb.h"
#include "supersonic/utils/strings/join.h"

namespace supersonic {

class BufferAllocator;

namespace {

// We create a separate class for MakeDatetime, as we don't have a unified
// framework for expressions with six arguments (and don't really need to have
// one).
// TODO(onufry): quiet and signaling versions could be added.
class BoundMakeDatetimeExpression : public BasicBoundExpression {
 public:
  BoundMakeDatetimeExpression(const string& name,
                              BufferAllocator* allocator,
                              unique_ptr<BoundExpression> year,
                              unique_ptr<BoundExpression> month,
                              unique_ptr<BoundExpression> day,
                              unique_ptr<BoundExpression> hour,
                              unique_ptr<BoundExpression> minute,
                              unique_ptr<BoundExpression> second)
      : BasicBoundExpression(CreateSchema(name, DATETIME, NULLABLE), allocator),
        year_(std::move(year)),
        month_(std::move(month)),
        day_(std::move(day)),
        hour_(std::move(hour)),
        minute_(std::move(minute)),
        second_(std::move(second)) {}

  virtual rowcount_t row_capacity() const {
    rowcount_t capacity = my_const_block()->row_capacity();
    for (int i = 0; i < 6; ++i) {
      capacity = std::min(capacity, child(i)->row_capacity());
    }
    return capacity;
  }

  virtual bool can_be_resolved() const {
    for (int i = 0; i < 6; ++i) if (!child(i)->is_constant()) return false;
    return true;
  }

  virtual EvaluationResult DoEvaluate(const View& input,
                                      const BoolView& skip_vectors) {
    CHECK_EQ(1, skip_vectors.column_count());
    // Usually we would call my_block()->ResetArenas(), but here we know the
    // block contains only DATETIME type data.
    bool_ptr skip_vector = skip_vectors.column(0);
    // Calculate results for children.
    vector<const View*> results;
    for (int i = 0; i < 6; ++i) {
      EvaluationResult result = child(i)->DoEvaluate(input, skip_vectors);
      PROPAGATE_ON_FAILURE(result);
      results.push_back(&result.get());
    }

    const int64_t* year = results[0]->column(0).typed_data<INT64>();
    const int64_t* month = results[1]->column(0).typed_data<INT64>();
    const int64_t* day = results[2]->column(0).typed_data<INT64>();
    const int64_t* hour = results[3]->column(0).typed_data<INT64>();
    const int64_t* minute = results[4]->column(0).typed_data<INT64>();
    const int64_t* second = results[5]->column(0).typed_data<INT64>();

    int64_t* destination =
        my_block()->mutable_column(0)->mutable_typed_data<DATETIME>();
    // We will implement MakeDatetime in terms of MakeDate, to avoid code
    // duplication.
    operators::MakeDate make_date;

    // We preform a checking evaluation, not because the evaluation is very
    // expensive (although it does cost a bit), but because we need to update
    // the skip_vector with new nulls anyway.
    for (int i = 0; i < input.row_count(); ++i) {
      if (*skip_vector == false) {
        int64_t result = make_date(year[i], month[i], day[i]);
        // If the calculation failed, we set the result to NULL. Makedate
        // signals a failure with a negative result.
        *skip_vector |= (result < 0LL);
        // We could break in the above case, but the calculations below are
        // quick enough they are probably not worth the branching.
        // We add the hours, minutes and seconds.
        result += (3600000000LL * hour[i] + 60000000LL * minute[i]
                   + 1000000LL * second[i]);
        *destination = result;
      }
      ++destination;
      ++skip_vector;
    }

    my_view()->set_row_count(input.row_count());
    my_view()->mutable_column(0)->ResetIsNull(skip_vectors.column(0));
    return Success(*my_view());
  }

  virtual void CollectReferredAttributeNames(
      set<string>* referred_attribute_names) const {
    for (int i = 0; i < 6; ++i) {
      child(i)->CollectReferredAttributeNames(referred_attribute_names);
    }
  }

 private:
  BoundExpression* child(size_t index) const {
    switch (index) {
      case 0: return year_.get();
      case 1: return month_.get();
      case 2: return day_.get();
      case 3: return hour_.get();
      case 4: return minute_.get();
      case 5: return second_.get();
      default: LOG(FATAL) << "Unexpected call to child in MakeDateTime, "
                          << "requesting child indexed " << index;
    }
    return NULL;
  }

  unique_ptr<BoundExpression> year_;
  unique_ptr<BoundExpression> month_;
  unique_ptr<BoundExpression> day_;
  unique_ptr<BoundExpression> hour_;
  unique_ptr<BoundExpression> minute_;
  unique_ptr<BoundExpression> second_;

  DISALLOW_COPY_AND_ASSIGN(BoundMakeDatetimeExpression);
};
}  // namespace

FailureOrOwned<BoundExpression> BoundUnixTimestamp(unique_ptr<BoundExpression> e,
                                                   BufferAllocator* allocator,
                                                   rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_UNIXTIMESTAMP, DATETIME,
    INT64>(allocator, max_row_count, std::move(e));
}

FailureOrOwned<BoundExpression> BoundFromUnixTime(unique_ptr<BoundExpression> e,
                                                  BufferAllocator* allocator,
                                                  rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_FROMUNIXTIME, INT64,
    DATETIME>(allocator, max_row_count, std::move(e));
}

FailureOrOwned<BoundExpression> BoundMakeDate(unique_ptr<BoundExpression> year,
                                              unique_ptr<BoundExpression> month,
                                              unique_ptr<BoundExpression> day,
                                              BufferAllocator* allocator,
                                              rowcount_t max_row_count) {
  return CreateTypedBoundTernaryExpression<OPERATOR_MAKEDATE, INT64, INT64,
                                           INT64, DATETIME>(
      allocator, max_row_count, std::move(year), std::move(month),
      std::move(day));
}

FailureOrOwned<BoundExpression> BoundYear(unique_ptr<BoundExpression> datetime,
                                          BufferAllocator* allocator,
                                          rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_YEAR, DATETIME, INT32>(
      allocator, max_row_count, std::move(datetime));
}

FailureOrOwned<BoundExpression> BoundQuarter(unique_ptr<BoundExpression> datetime,
                                             BufferAllocator* allocator,
                                             rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_QUARTER, DATETIME, INT32>(
      allocator, max_row_count, std::move(datetime));
}

FailureOrOwned<BoundExpression> BoundMonth(unique_ptr<BoundExpression> datetime,
                                           BufferAllocator* allocator,
                                           rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_MONTH, DATETIME, INT32>(
      allocator, max_row_count, std::move(datetime));
}

FailureOrOwned<BoundExpression> BoundDay(unique_ptr<BoundExpression> datetime,
                                         BufferAllocator* allocator,
                                         rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_DAY, DATETIME, INT32>(
      allocator, max_row_count, std::move(datetime));
}

FailureOrOwned<BoundExpression> BoundWeekday(unique_ptr<BoundExpression> datetime,
                                             BufferAllocator* allocator,
                                             rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_WEEKDAY, DATETIME, INT32>(
      allocator, max_row_count, std::move(datetime));
}

FailureOrOwned<BoundExpression> BoundYearDay(unique_ptr<BoundExpression> datetime,
                                             BufferAllocator* allocator,
                                             rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_YEARDAY, DATETIME, INT32>(
      allocator, max_row_count, std::move(datetime));
}

FailureOrOwned<BoundExpression> BoundHour(unique_ptr<BoundExpression> datetime,
                                          BufferAllocator* allocator,
                                          rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_HOUR, DATETIME, INT32>(
      allocator, max_row_count, std::move(datetime));
}

FailureOrOwned<BoundExpression> BoundMinute(unique_ptr<BoundExpression> datetime,
                                            BufferAllocator* allocator,
                                            rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_MINUTE, DATETIME, INT32>(
      allocator, max_row_count, std::move(datetime));
}

FailureOrOwned<BoundExpression> BoundSecond(unique_ptr<BoundExpression> datetime,
                                            BufferAllocator* allocator,
                                            rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_SECOND, DATETIME, INT32>(
      allocator, max_row_count, std::move(datetime));
}

FailureOrOwned<BoundExpression> BoundMicrosecond(unique_ptr<BoundExpression> datetime,
                                                 BufferAllocator* allocator,
                                                 rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_MICROSECOND, DATETIME, INT32>(
      allocator, max_row_count, std::move(datetime));
}

FailureOrOwned<BoundExpression> BoundYearLocal(unique_ptr<BoundExpression> datetime,
                                               BufferAllocator* allocator,
                                               rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_YEAR_LOCAL, DATETIME, INT32>(
      allocator, max_row_count, std::move(datetime));
}

FailureOrOwned<BoundExpression> BoundQuarterLocal(unique_ptr<BoundExpression> datetime,
                                                  BufferAllocator* allocator,
                                                  rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_QUARTER_LOCAL, DATETIME,
      INT32>(allocator, max_row_count, std::move(datetime));
}
FailureOrOwned<BoundExpression> BoundMonthLocal(unique_ptr<BoundExpression> datetime,
                                                BufferAllocator* allocator,
                                                rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_MONTH_LOCAL, DATETIME, INT32>(
      allocator, max_row_count, std::move(datetime));
}
FailureOrOwned<BoundExpression> BoundDayLocal(unique_ptr<BoundExpression> datetime,
                                              BufferAllocator* allocator,
                                              rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_DAY_LOCAL, DATETIME, INT32>(
      allocator, max_row_count, std::move(datetime));
}
FailureOrOwned<BoundExpression> BoundWeekdayLocal(unique_ptr<BoundExpression> datetime,
                                                  BufferAllocator* allocator,
                                                  rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_WEEKDAY_LOCAL, DATETIME,
      INT32>(allocator, max_row_count, std::move(datetime));
}
FailureOrOwned<BoundExpression> BoundYearDayLocal(unique_ptr<BoundExpression> datetime,
                                                  BufferAllocator* allocator,
                                                  rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_YEARDAY_LOCAL, DATETIME,
      INT32>(allocator, max_row_count, std::move(datetime));
}
FailureOrOwned<BoundExpression> BoundHourLocal(unique_ptr<BoundExpression> datetime,
                                               BufferAllocator* allocator,
                                               rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_HOUR_LOCAL, DATETIME, INT32>(
      allocator, max_row_count, std::move(datetime));
}
FailureOrOwned<BoundExpression> BoundMinuteLocal(unique_ptr<BoundExpression> datetime,
                                                 BufferAllocator* allocator,
                                                 rowcount_t max_row_count) {
  return CreateTypedBoundUnaryExpression<OPERATOR_MINUTE_LOCAL, DATETIME,
      INT32>(allocator, max_row_count, std::move(datetime));
}

FailureOrOwned<BoundExpression> BoundMakeDatetime(unique_ptr<BoundExpression> year,
                                                  unique_ptr<BoundExpression> month,
                                                  unique_ptr<BoundExpression> day,
                                                  unique_ptr<BoundExpression> hour,
                                                  unique_ptr<BoundExpression> minute,
                                                  unique_ptr<BoundExpression> second,
                                                  BufferAllocator* allocator,
                                                  rowcount_t max_row_count) {
  string name = StrCat("MAKE_DATETIME(", GetExpressionName(year.get()),  ", ",
                       GetExpressionName(month.get()), ", ", GetExpressionName(day.get()),
                       StrCat(", ", GetExpressionName(hour.get()), ", ",
                              GetExpressionName(minute.get()), ", ",
                              GetExpressionName(second.get()), ")"));

  FailureOrOwned<BoundExpression> year_result =
      BoundInternalCast(allocator, max_row_count, std::move(year), INT64, true);
  FailureOrOwned<BoundExpression> month_result =
      BoundInternalCast(allocator, max_row_count, std::move(month), INT64, true);
  FailureOrOwned<BoundExpression> day_result =
      BoundInternalCast(allocator, max_row_count, std::move(day), INT64, true);
  FailureOrOwned<BoundExpression> hour_result =
      BoundInternalCast(allocator, max_row_count, std::move(hour), INT64, true);
  FailureOrOwned<BoundExpression> minute_result =
      BoundInternalCast(allocator, max_row_count, std::move(minute), INT64, true);
  FailureOrOwned<BoundExpression> second_result =
      BoundInternalCast(allocator, max_row_count, std::move(second), INT64, true);
  // We first process all the inputs, and only then propagate failures, as
  // a side-effect of the processing is taking ownership, and we don't want the
  // inputs to leak in case of a failure.
  PROPAGATE_ON_FAILURE(year_result);
  PROPAGATE_ON_FAILURE(month_result);
  PROPAGATE_ON_FAILURE(day_result);
  PROPAGATE_ON_FAILURE(hour_result);
  PROPAGATE_ON_FAILURE(minute_result);
  PROPAGATE_ON_FAILURE(second_result);

  return InitBasicExpression(
      max_row_count,
      make_unique<BoundMakeDatetimeExpression>(
          name, allocator, year_result.move(), month_result.move(),
          day_result.move(), hour_result.move(), minute_result.move(),
          second_result.move()),
      allocator);
}

FailureOrOwned<BoundExpression> BoundDateFormat(unique_ptr<BoundExpression> datetime,
                                                unique_ptr<BoundExpression> format,
                                                BufferAllocator* allocator,
                                                rowcount_t max_row_count) {
  return CreateTypedBoundBinaryExpression<OPERATOR_DATEFORMAT, DATETIME, STRING,
      STRING>(allocator, max_row_count, std::move(datetime), std::move(format));
}

FailureOrOwned<BoundExpression> BoundDateFormatLocal(unique_ptr<BoundExpression> datetime,
                                                     unique_ptr<BoundExpression> format,
                                                     BufferAllocator* allocator,
                                                     rowcount_t max_row_count) {
  return CreateTypedBoundBinaryExpression<OPERATOR_DATEFORMAT_LOCAL, DATETIME,
      STRING, STRING>(allocator, max_row_count, std::move(datetime), std::move(format));
}

}  // namespace supersonic
