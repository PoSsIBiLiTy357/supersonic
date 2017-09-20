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
// Author: onufry@google.com (Onufry Wojtaszczyk)
//
// Expressions on DATE and DATETIME, bound versions. See date_expressions.h for
// semantics.

#ifndef SUPERSONIC_EXPRESSION_CORE_DATE_BOUND_EXPRESSIONS_H_
#define SUPERSONIC_EXPRESSION_CORE_DATE_BOUND_EXPRESSIONS_H_



#include "supersonic/base/exception/result.h"
#include "supersonic/base/infrastructure/types.h"

namespace supersonic {

// Returns the INT64 timestamp (microseconds from January 1st, 1970) of
// the specified date. Returns NULL if the date is NULL.
class BoundExpression;
class BufferAllocator;

FailureOrOwned<BoundExpression> BoundUnixTimestamp(unique_ptr<BoundExpression> date,
                                                   BufferAllocator* allocator,
                                                   rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundFromUnixTime(unique_ptr<BoundExpression> timestamp,
                                                  BufferAllocator* allocator,
                                                  rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundMakeDate(unique_ptr<BoundExpression> year,
                                              unique_ptr<BoundExpression> month,
                                              unique_ptr<BoundExpression> day,
                                              BufferAllocator* allocator,
                                              rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundMakeDatetime(unique_ptr<BoundExpression> year,
                                                  unique_ptr<BoundExpression> month,
                                                  unique_ptr<BoundExpression> day,
                                                  unique_ptr<BoundExpression> hour,
                                                  unique_ptr<BoundExpression> minute,
                                                  unique_ptr<BoundExpression> second,
                                                  BufferAllocator* allocator,
                                                  rowcount_t max_row_count);

// Date specifics retrieval - UTC versions.
FailureOrOwned<BoundExpression> BoundYear(unique_ptr<BoundExpression> datetime,
                                          BufferAllocator* allocator,
                                          rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundQuarter(unique_ptr<BoundExpression> datetime,
                                             BufferAllocator* allocator,
                                             rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundMonth(unique_ptr<BoundExpression> datetime,
                                           BufferAllocator* allocator,
                                           rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundDay(unique_ptr<BoundExpression> datetime,
                                         BufferAllocator* allocator,
                                         rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundWeekday(unique_ptr<BoundExpression> datetime,
                                             BufferAllocator* allocator,
                                             rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundYearDay(unique_ptr<BoundExpression> datetime,
                                             BufferAllocator* allocator,
                                             rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundHour(unique_ptr<BoundExpression> datetime,
                                          BufferAllocator* allocator,
                                          rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundMinute(unique_ptr<BoundExpression> datetime,
                                            BufferAllocator* allocator,
                                            rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundSecond(unique_ptr<BoundExpression> datetime,
                                            BufferAllocator* allocator,
                                            rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundMicrosecond(unique_ptr<BoundExpression> datetime,
                                                 BufferAllocator* allocator,
                                                 rowcount_t max_row_count);

// Date specifics retrieval - local timezone versions. Deprecated.
FailureOrOwned<BoundExpression> BoundYearLocal(unique_ptr<BoundExpression> datetime,
                                               BufferAllocator* allocator,
                                               rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundQuarterLocal(unique_ptr<BoundExpression> datetime,
                                                  BufferAllocator* allocator,
                                                  rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundMonthLocal(unique_ptr<BoundExpression> datetime,
                                                BufferAllocator* allocator,
                                                rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundDayLocal(unique_ptr<BoundExpression> datetime,
                                              BufferAllocator* allocator,
                                              rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundWeekdayLocal(unique_ptr<BoundExpression> datetime,
                                                  BufferAllocator* allocator,
                                                  rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundYearDayLocal(unique_ptr<BoundExpression> datetime,
                                                  BufferAllocator* allocator,
                                                  rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundHourLocal(unique_ptr<BoundExpression> datetime,
                                               BufferAllocator* allocator,
                                               rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundMinuteLocal(unique_ptr<BoundExpression> datetime,
                                                 BufferAllocator* allocator,
                                                 rowcount_t max_row_count);

// Dateformat expressions.
FailureOrOwned<BoundExpression> BoundDateFormat(unique_ptr<BoundExpression> datetime,
                                                unique_ptr<BoundExpression> format,
                                                BufferAllocator* allocator,
                                                rowcount_t max_row_count);

FailureOrOwned<BoundExpression> BoundDateFormatLocal(unique_ptr<BoundExpression> datetime,
                                                     unique_ptr<BoundExpression> format,
                                                     BufferAllocator* allocator,
                                                     rowcount_t max_row_count);

}  // namespace supersonic

#endif  // SUPERSONIC_EXPRESSION_CORE_DATE_BOUND_EXPRESSIONS_H_
