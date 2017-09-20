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

#include "supersonic/expression/core/date_expressions.h"

#include "supersonic/utils/std_namespace.h"
#include "supersonic/utils/walltime.h"
#include "supersonic/base/exception/exception.h"
#include "supersonic/base/exception/exception_macros.h"
#include "supersonic/base/exception/result.h"
#include "supersonic/base/infrastructure/types.h"
#include "supersonic/expression/base/expression.h"
#include "supersonic/expression/core/date_bound_expressions.h"
#include "supersonic/expression/core/date_evaluators.h"  // IWYU pragma: keep
#include "supersonic/expression/core/elementary_expressions.h"
#include "supersonic/expression/infrastructure/basic_expressions.h"
#include "supersonic/expression/infrastructure/terminal_expressions.h"
#include "supersonic/expression/proto/operators.pb.h"
#include "supersonic/expression/templated/abstract_expressions.h"
#include "supersonic/proto/supersonic.pb.h"
#include "supersonic/utils/strings/join.h"

namespace supersonic {

class BufferAllocator;
class TupleSchema;

namespace {

// The MakeDatetime expression. We don't have a senary expression framework, so
// this is implemented separately.
class MakeDatetimeExpression : public Expression {
 public:
  MakeDatetimeExpression(unique_ptr<const Expression> year,
                         unique_ptr<const Expression> month,
                         unique_ptr<const Expression> day,
                         unique_ptr<const Expression> hour,
                         unique_ptr<const Expression> minute,
                         unique_ptr<const Expression> second)
      : year_(std::move(year)),
        month_(std::move(month)),
        day_(std::move(day)),
        hour_(std::move(hour)),
        minute_(std::move(minute)),
        second_(std::move(second)) {}
 private:
  virtual FailureOrOwned<BoundExpression> DoBind(
      const TupleSchema& input_schema,
      BufferAllocator* allocator,
      rowcount_t max_row_count) const {
    FailureOrOwned<BoundExpression> bound_year = year_->DoBind(
        input_schema, allocator, max_row_count);
    PROPAGATE_ON_FAILURE(bound_year);

    FailureOrOwned<BoundExpression> bound_month = month_->DoBind(
        input_schema, allocator, max_row_count);
    PROPAGATE_ON_FAILURE(bound_month);

    FailureOrOwned<BoundExpression> bound_day = day_->DoBind(
        input_schema, allocator, max_row_count);
    PROPAGATE_ON_FAILURE(bound_day);

    FailureOrOwned<BoundExpression> bound_hour = hour_->DoBind(
        input_schema, allocator, max_row_count);
    PROPAGATE_ON_FAILURE(bound_hour);

    FailureOrOwned<BoundExpression> bound_minute = minute_->DoBind(
        input_schema, allocator, max_row_count);
    PROPAGATE_ON_FAILURE(bound_minute);

    FailureOrOwned<BoundExpression> bound_second = second_->DoBind(
        input_schema, allocator, max_row_count);
    PROPAGATE_ON_FAILURE(bound_second);

    return BoundMakeDatetime(bound_year.move(), bound_month.move(),
                             bound_day.move(), bound_hour.move(),
                             bound_minute.move(), bound_second.move(),
                             allocator, max_row_count);
  }

  virtual string ToString(bool verbose) const {
    // StrCat takes at most 12 arguments, which necessitates the double call.
    return StrCat(
        StrCat("MAKE_DATETIME(", year_->ToString(verbose), ", ",
               month_->ToString(verbose), ", ", day_->ToString(verbose)),
        ", ", hour_->ToString(verbose), ", ", minute_->ToString(verbose),
        ", ", second_->ToString(verbose), ")");
  }

  const unique_ptr<const Expression> year_;
  const unique_ptr<const Expression> month_;
  const unique_ptr<const Expression> day_;
  const unique_ptr<const Expression> hour_;
  const unique_ptr<const Expression> minute_;
  const unique_ptr<const Expression> second_;
};

}  // namespace

unique_ptr<const Expression> ConstDateTime(const StringPiece& value) {
  // This implementation looks bad, because it looks like the resolving of the
  // parsing will be performed at evaluation time, for each row separately.
  // Fortunately, we do have constant folding in Supersonic, so actually this
  // calculation will be performed once, during binding time, and the expression
  // will be collapsed to some const DateTime.
  return ParseStringNulling(DATETIME, ConstString(value));
}

unique_ptr<const Expression> ConstDateTimeFromMicrosecondsSinceEpoch(const int64& value) {
  return ConstDateTime(value);
}

unique_ptr<const Expression> ConstDateTimeFromSecondsSinceEpoch(const double& value) {
  return ConstDateTime(static_cast<int64>(value * 1000000.));
}

unique_ptr<const Expression> Now() {
  return ConstDateTime(static_cast<int64>(WallTime_Now() * 1000000.));
}

unique_ptr<const Expression> UnixTimestamp(unique_ptr<const Expression> datetime) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), &BoundUnixTimestamp, "UNIXTIMESTAMP($0)");
}

unique_ptr<const Expression> FromUnixTime(unique_ptr<const Expression> timestamp) {
  return CreateExpressionForExistingBoundFactory(
      std::move(timestamp), &BoundFromUnixTime, "FROMUNIXTIME($0)");
}

unique_ptr<const Expression> MakeDate(unique_ptr<const Expression> year,
                                      unique_ptr<const Expression> month,
                                      unique_ptr<const Expression> day) {
  return CreateExpressionForExistingBoundFactory(
      std::move(year), std::move(month), std::move(day), &BoundMakeDate,
      "MAKEDATE($0, $1, $2)");
}

unique_ptr<const Expression> MakeDatetime(unique_ptr<const Expression> year,
                               unique_ptr<const Expression> month,
                               unique_ptr<const Expression> day,
                               unique_ptr<const Expression> hour,
                               unique_ptr<const Expression> minute,
                               unique_ptr<const Expression> second) {
  return make_unique<MakeDatetimeExpression>(
      std::move(year),
      std::move(month),
      std::move(day),
      std::move(hour),
      std::move(minute),
      std::move(second));
}

unique_ptr<const Expression> Year(unique_ptr<const Expression> datetime) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), &BoundYear, "YEAR($0)");
}

unique_ptr<const Expression> Quarter(unique_ptr<const Expression> datetime) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), &BoundQuarter, "QUARTER($0)");
}

unique_ptr<const Expression> Month(unique_ptr<const Expression> datetime) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), &BoundMonth, "MONTH($0)");
}

unique_ptr<const Expression> Day(unique_ptr<const Expression> datetime) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), &BoundDay, "DAY($0)");
}

unique_ptr<const Expression> AddMinutes(unique_ptr<const Expression> datetime,
                             unique_ptr<const Expression> number_of_minutes) {
  return make_unique<TypedBinaryExpression<OPERATOR_ADD_MINUTES, DATETIME,
      INT64, DATETIME>>(std::move(datetime), std::move(number_of_minutes));
}

unique_ptr<const Expression> AddMinute(unique_ptr<const Expression> datetime) {
  return AddMinutes(std::move(datetime), ConstInt64(1LL));
}

unique_ptr<const Expression> AddDays(unique_ptr<const Expression> datetime,
                          unique_ptr<const Expression> number_of_days) {
  return make_unique<TypedBinaryExpression<OPERATOR_ADD_DAYS, DATETIME,
      INT64, DATETIME>>(std::move(datetime), std::move(number_of_days));
}

unique_ptr<const Expression> AddDay(unique_ptr<const Expression> datetime) {
  return AddDays(std::move(datetime), ConstInt64(1LL));
}

unique_ptr<const Expression> AddMonths(unique_ptr<const Expression> datetime,
                                  unique_ptr<const Expression> number_of_months) {
  return make_unique<TypedBinaryExpression<OPERATOR_ADD_MONTHS, DATETIME,
      INT64, DATETIME>>(std::move(datetime), std::move(number_of_months));
}

unique_ptr<const Expression> AddMonth(unique_ptr<const Expression> datetime) {
  return AddMonths(std::move(datetime), ConstInt64(1LL));
}

unique_ptr<const Expression> Weekday(unique_ptr<const Expression> datetime) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), &BoundWeekday, "WEEKDAY($0)");
}

unique_ptr<const Expression> YearDay(unique_ptr<const Expression> datetime) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), &BoundYearDay, "YEARDAY($0)");
}

unique_ptr<const Expression> Hour(unique_ptr<const Expression> datetime) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), &BoundHour, "HOUR($0)");
}

unique_ptr<const Expression> Minute(unique_ptr<const Expression> datetime) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), &BoundMinute, "MINUTE($0)");
}

unique_ptr<const Expression> Second(unique_ptr<const Expression> datetime) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), &BoundSecond, "SECOND($0)");
}

unique_ptr<const Expression> Microsecond(unique_ptr<const Expression> datetime) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), &BoundMicrosecond, "MICROSECOND($0)");
}

unique_ptr<const Expression> YearLocal(unique_ptr<const Expression> datetime) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), &BoundYearLocal, "YEAR_LOCAL($0)");
}

unique_ptr<const Expression> QuarterLocal(unique_ptr<const Expression> datetime) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), &BoundQuarterLocal, "QUARTER_LOCAL($0)");
}

unique_ptr<const Expression> MonthLocal(unique_ptr<const Expression> datetime) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), &BoundMonthLocal, "MONTH_LOCAL($0)");
}

unique_ptr<const Expression> DayLocal(unique_ptr<const Expression> datetime) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), &BoundDayLocal, "DAY_LOCAL($0)");
}

unique_ptr<const Expression> WeekdayLocal(unique_ptr<const Expression> datetime) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), &BoundWeekdayLocal, "WEEKDAY_LOCAL($0)");
}

unique_ptr<const Expression> YearDayLocal(unique_ptr<const Expression> datetime) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), &BoundYearDayLocal, "YEAR_DAY_LOCAL($0)");
}

unique_ptr<const Expression> HourLocal(unique_ptr<const Expression> datetime) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), &BoundHourLocal, "HOUR_LOCAL($0)");
}

unique_ptr<const Expression> MinuteLocal(unique_ptr<const Expression> datetime) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), &BoundMinuteLocal, "MINUTE_LOCAL($0)");
}

unique_ptr<const Expression> SecondLocal(unique_ptr<const Expression> datetime) {
  return Second(std::move(datetime));
}

unique_ptr<const Expression> MicrosecondLocal(unique_ptr<const Expression> datetime) {
  return Microsecond(std::move(datetime));
}

unique_ptr<const Expression> DateFormat(unique_ptr<const Expression> datetime,
                             unique_ptr<const Expression> format) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), std::move(format), &BoundDateFormat, "DATE_FORMAT($0, $1)");
}

unique_ptr<const Expression> DateFormatLocal(unique_ptr<const Expression> datetime,
                                  unique_ptr<const Expression> format) {
  return CreateExpressionForExistingBoundFactory(
      std::move(datetime), std::move(format), &BoundDateFormatLocal, "DATE_FORMAT_LOCAL($0, $1)");
}

}  // namespace supersonic
