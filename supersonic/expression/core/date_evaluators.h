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
// Author: onufry@google.com (Onufry Wojtaszczyk)
//
// Definitions of the individual operations for expressions defined in
// date_expressions.h

#ifndef SUPERSONIC_EXPRESSION_CORE_DATE_EVALUATORS_H_
#define SUPERSONIC_EXPRESSION_CORE_DATE_EVALUATORS_H_

#include <cstddef>

#include "supersonic/utils/integral_types.h"
#include "supersonic/base/infrastructure/bit_pointers.h"
#include "supersonic/utils/strings/stringpiece.h"

namespace supersonic {

class Arena;

namespace operators {

static const int64_t kMillion = 1000000LL;

// Returns a datetime from a unix timestamp.
// We store the datetime as timestamps already, only calculated in microseconds,
// not seconds.
struct FromUnixTime {
  int64_t operator()(const int64_t arg) const { return arg * kMillion; }
};

// Returns unix timestamp from datetime.
// We store the datetime as timestamps already, only calculated in microseconds,
// not seconds.
struct UnixTimestamp {
  int64_t operator()(const int64_t arg) const { return arg / kMillion; }
};

struct MakeDate {
  int64_t operator()(const int64_t year, const int64_t month, const int64_t day);
};

struct Year {
  int32_t operator()(const int64_t datetime);
};

struct YearLocal {
  int32_t operator()(const int64_t datetime);
};

struct Quarter {
  int32_t operator()(const int64_t datetime);
};

struct QuarterLocal {
  int32_t operator()(const int64_t datetime);
};

struct Month {
  int32_t operator()(const int64_t datetime);
};

struct MonthLocal {
  int32_t operator()(const int64_t datetime);
};

struct Day {
  int32_t operator()(const int64_t datetime);
};

struct DayLocal {
  int32_t operator()(const int64_t datetime);
};

struct Weekday {
  int32_t operator()(const int64_t datetime);
};

struct WeekdayLocal {
  int32_t operator()(const int64_t datetime);
};

struct YearDay {
  int32_t operator()(const int64_t datetime);
};

struct YearDayLocal {
  int32_t operator()(const int64_t datetime);
};

struct Hour {
  int32_t operator()(const int64_t datetime);
};

struct HourLocal {
  int32_t operator()(const int64_t datetime);
};

struct Minute {
  int32_t operator()(const int64_t datetime);
};

struct MinuteLocal {
  int32_t operator()(const int64_t datetime);
};

struct Second {
  int32_t operator()(const int64_t datetime);
};

struct Microsecond {
  int32_t operator()(const int64_t datetime);
};

struct AddMinutes {
  int64_t operator()(const int64_t datetime, const int64_t number_of_minutes) {
    return datetime + number_of_minutes * 60LL * kMillion;
  }
};

struct AddDays {
  int64_t operator()(const int64_t datetime, const int64_t number_of_days) {
    return datetime + number_of_days * 24LL * 3600LL * kMillion;
  }
};

struct AddMonths {
  int64_t operator()(const int64_t datetime, const int64_t number_of_months);
};

// Note - this is not INT64-compliant.
struct DateFormat {
  StringPiece operator()(int64_t datetime, const StringPiece& format,
                         Arena* arena);
};

struct DateFormatLocal {
  StringPiece operator()(int64_t datetime, const StringPiece& format,
                         Arena* arena);
};

}  // namespace operators
namespace failers {

// Returns the number of rows where the MakeDate function fails.
struct MakeDateFailer {
  int operator()(const int64_t* left_data,
                 bool_const_ptr left_is_null,
                 const int64_t* middle_data,
                 bool_const_ptr middle_is_null,
                 const int64_t* right_data,
                 bool_const_ptr right_is_null,
                 size_t row_count);
};

}  // namespace failers
}  // namespace supersonic

#endif  // SUPERSONIC_EXPRESSION_CORE_DATE_EVALUATORS_H_
