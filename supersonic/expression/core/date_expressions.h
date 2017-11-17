// Copyright 2010 Google Inc.  All Rights Reserved
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
// Expressions on DATE and DATETIME.

#ifndef SUPERSONIC_EXPRESSION_CORE_DATE_EXPRESSIONS_H_
#define SUPERSONIC_EXPRESSION_CORE_DATE_EXPRESSIONS_H_

#include "supersonic/utils/integral_types.h"
#include "supersonic/utils/strings/stringpiece.h"

namespace supersonic {

class Expression;

// Parses a datetime in the format YYYY/MM/DD-HH:MM:SS interpreted
// as UTC. If parsing fails, returns Null(DATETIME).
// TODO(user): Support for format?
unique_ptr<const Expression> ConstDateTime(const StringPiece &value);

// Creates a DATETIME constant from the given number of microseconds
// since January 1st, 1970 00:00 UTC.
unique_ptr<const Expression>
ConstDateTimeFromMicrosecondsSinceEpoch(const int64_t &value);

// Creates a DATETIME constant from the given number of seconds
// since January 1st, 1970 00:00 UTC.
unique_ptr<const Expression>
ConstDateTimeFromSecondsSinceEpoch(const double &value);

// Creates a DATETIME constant from the current time (i.e., the time
// at the call of Now() and not when the expression is evaluated).
unique_ptr<const Expression> Now();

// Returns the INT64 timestamp (microseconds from January 1st, 1970) of
// the specified date. Returns NULL if the date is NULL.
unique_ptr<const Expression> UnixTimestamp(unique_ptr<const Expression> date);

// Returns a DATETIME from the given timestamp (microseconds from Jan 1st,
// 1970). Returns NULL if the input is NULL.
unique_ptr<const Expression>
FromUnixTime(unique_ptr<const Expression> timestamp);

// Returns a DATETIME from the given year, month and day. This is timezone
// independent (or, in other words, it returns the DATETIME corresponding to
// the UTC 0:00 of that date).
unique_ptr<const Expression> MakeDate(unique_ptr<const Expression> year,
                                      unique_ptr<const Expression> month,
                                      unique_ptr<const Expression> day);

// Returns a DATETIME from the given year, month, day, hour, minute and second.
// This is timezone independent (or, in other words, returns the DATETIME in
// UTC).
unique_ptr<const Expression> MakeDatetime(unique_ptr<const Expression> year,
                                          unique_ptr<const Expression> month,
                                          unique_ptr<const Expression> day,
                                          unique_ptr<const Expression> hour,
                                          unique_ptr<const Expression> minute,
                                          unique_ptr<const Expression> second);

// A few frequently used date formats for use in ParseDateTime().
// "%a, %d %b %Y %H:%M:%S GMT", e.g. Sat, 24 May 2008 20:09:47 GMT
extern const char *const kDateFormatRfc1123;

// "%Y-%m-%dT%H:%M:%SZ", e.g. 2008-05-24T20:09:47Z
extern const char *const kDateFormatRfc3339;

// "%Y/%m/%d-%H:%M:%S", e.g. 2008/05/24-20:09:47
extern const char *const kDateFormatDefault;

// Creates an expression that will parse VARCHAR to a DATETIME using
// the given format.  See man strptime() for the format specification.
// NULLs, unparsable strings, bad format, out of range will be converted
// to NULLs. Assumes the datetime is given in UTC.
// Whitespace at either end of the string to be parsed is accepted.
unique_ptr<const Expression> ParseDateTime(const StringPiece &format,
                                           unique_ptr<const Expression> e);
// For PrintDateTime see either DateFormat in this file, or ToString (in
// string_expressions.h).

// Datetime specifics extraction. All of them return INT32.
// The number of years AD.
unique_ptr<const Expression> Year(unique_ptr<const Expression> e);

// The quarter, in the range 1 to 4.
unique_ptr<const Expression> Quarter(unique_ptr<const Expression> e);

// The month of the year, in the range 1 to 12.
unique_ptr<const Expression> Month(unique_ptr<const Expression> e);

//  The day of the month, in the range 1 to 31.
unique_ptr<const Expression> Day(unique_ptr<const Expression> e);

// The number of days since Monday, in the range 0 to 6.
unique_ptr<const Expression> Weekday(unique_ptr<const Expression> e);

// The number of days into the year, in the range 1 to 366.
unique_ptr<const Expression> YearDay(unique_ptr<const Expression> e);

// The number of hours past midnight, in the range 0 to 23.
unique_ptr<const Expression> Hour(unique_ptr<const Expression> e);

// The number of minutes after the hour, in the range 0 to 59.
unique_ptr<const Expression> Minute(unique_ptr<const Expression> e);

// The  number of seconds after the minute, normally in the range 0
// to 59, but can be up to 60 to allow for leap seconds.
unique_ptr<const Expression> Second(unique_ptr<const Expression> e);

// The number of microseconds after the second, in the range 0 to 999999.
unique_ptr<const Expression> Microsecond(unique_ptr<const Expression> e);

// The local functions have semantics as above, with the exception that they use
// the local timezone settings instead of UTC. As this generates problems and
// inconsistencies, they should be considered deprecated from the beginning, and
// we will push to withdraw them, they are added only for the purpose of
// these are needed, please file a bug against the Supersonic team, and we will
// provide versions which take the timezone as an argument.
unique_ptr<const Expression> YearLocal(unique_ptr<const Expression> e);
unique_ptr<const Expression> QuarterLocal(unique_ptr<const Expression> e);
unique_ptr<const Expression> MonthLocal(unique_ptr<const Expression> e);
unique_ptr<const Expression> DayLocal(unique_ptr<const Expression> e);
unique_ptr<const Expression> WeekdayLocal(unique_ptr<const Expression> e);
unique_ptr<const Expression> YearDayLocal(unique_ptr<const Expression> e);
unique_ptr<const Expression> HourLocal(unique_ptr<const Expression> e);
unique_ptr<const Expression> MinuteLocal(unique_ptr<const Expression> e);
// The last two are identical to their non-local counterparts, given here for
// completeness.
unique_ptr<const Expression> SecondLocal(unique_ptr<const Expression> e);
unique_ptr<const Expression> MicrosecondLocal(unique_ptr<const Expression> e);

// Add one minute to the current datetime (in UTC).
unique_ptr<const Expression>
AddMinute(unique_ptr<const Expression> const datetime);

// Adds the specified number of minutes to the current datetime (in UTC).
unique_ptr<const Expression>
AddMinutes(unique_ptr<const Expression> const datetime,
           unique_ptr<const Expression> const number_of_minutes);

// Add one day to the current datetime (in UTC). Note that if daylight savings
// hits during this day in some timezone, adding a day in this timezone is _not_
// equivalent to adding a day in UTC.
unique_ptr<const Expression>
AddDay(unique_ptr<const Expression> const datetime);

// Adds the specified number of days to the current datetime (in UTC).
unique_ptr<const Expression>
AddDays(unique_ptr<const Expression> const datetime,
        unique_ptr<const Expression> const number_of_days);

// Add one month to the current datetime (in UTC).
unique_ptr<const Expression>
AddMonth(unique_ptr<const Expression> const datetime);

// Add the specified number of months to the current datetime (in UTC).
unique_ptr<const Expression>
AddMonths(unique_ptr<const Expression> const datetime,
          unique_ptr<const Expression> const number_of_months);

// Outputs the formatted date. The format string is interpreted exactly as
// in strftime. Max length of output string is 30 chars, if the output string
// would be longer, an empty string is returned instead.
//
// The input is expected to be of type DATETIME or DATE (if it is of type DATE,
// it is interpreted as 00:00:00 of that date).
//
// WARNING: this function is not 64-bit-safe, i.e., it may return wrong values
// for timestamps not fitting in 32 bits.
unique_ptr<const Expression> DateFormat(unique_ptr<const Expression> datetime,
                                        unique_ptr<const Expression> format);

// The same as above, except it returns the date in the local timezone instead
// of UTC. Should be considered deprecated (due to various consistency reasons),
// if a timezone-aware version is necessary, please file a bug against the
// Supersonic team and we will add a version that takes the timezone as an
unique_ptr<const Expression>
DateFormatLocal(unique_ptr<const Expression> datetime,
                unique_ptr<const Expression> format);

} // namespace supersonic

#endif // SUPERSONIC_EXPRESSION_CORE_DATE_EXPRESSIONS_H_
