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
// Expressions on strings.

#ifndef SUPERSONIC_EXPRESSION_CORE_STRING_EXPRESSIONS_H_
#define SUPERSONIC_EXPRESSION_CORE_STRING_EXPRESSIONS_H_

#include "supersonic/utils/std_namespace.h"
#include "supersonic/utils/strings/stringpiece.h"
#include "supersonic/expression/base/expression.h"

namespace supersonic {

// Creates an expression that will convert any expression to VARCHAR.
unique_ptr<const Expression> ToString(unique_ptr<const Expression> arg);

// The ParseString expressions are in elementary_expressions.h.

// Concatenates the specified input. Converts all arguments to VARCHAR if
// they are not already.
unique_ptr<const Expression> Concat(unique_ptr<const ExpressionList> arguments);

// Concatenates the specified input, using the specified string as a separator.
// Converts all arguments to VARCHAR if they are not already.
//
// Currently not implemented.
unique_ptr<const Expression> ConcatWithSeparator(
    const StringPiece& separator,
    unique_ptr<const ExpressionList> arguments);

// Computes the length of the specified string.
// Returns NULL if the string is NULL.
unique_ptr<const Expression> Length(unique_ptr<const Expression> str);

// Removes white spaces from the left side of the specified string.
// Returns NULL if the string is NULL.
unique_ptr<const Expression> Ltrim(unique_ptr<const Expression> str);

// Removes white spaces from the right side of the specified string.
// Returns NULL if the string is NULL.
unique_ptr<const Expression> Rtrim(unique_ptr<const Expression> str);

// Removes white spaces from both sides of the specified string.
// Returns NULL if the string is NULL.
unique_ptr<const Expression> Trim(unique_ptr<const Expression> str);

// Converts the specified string to upper case or lower case, respectively.
// Returns NULL if the string is NULL.
unique_ptr<const Expression> ToUpper(unique_ptr<const Expression> str);
unique_ptr<const Expression> ToLower(unique_ptr<const Expression> str);

// Returns a substring starting at the position determined by the 'pos'
// argument. Returns NULL if either the string or the pos argument evaluate
// to NULL.
// Non-positive arguments are interpreted by the "from the end" semantics, see
// below.
unique_ptr<const Expression> TrailingSubstring(
    unique_ptr<const Expression> str,
    unique_ptr<const Expression> pos);

// Returns a substring starting at the position determined by the 'pos'
// argument, and at most 'length' bytes long. Returns NULL if either the
// string, pos, or length arguments evaluate to NULL.
// One-based (i.e., Substring("Cow", 2, 2) = "ow").
// Negative length is interpreted as zero.
// Negative pos is interpreted as "count from the end" (python-like semantics),
// thus Substring("Cow", -1, 1) = "w".
// Substring(str, 0, len) always returns an empty string (as in MySQL).
unique_ptr<const Expression> Substring(
    unique_ptr<const Expression> str,
    unique_ptr<const Expression> pos,
    unique_ptr<const Expression> length);

// Returns the first index (1-based) that is a beginning of needle in haystack,
// or zero if needle does not appear in haystack.
unique_ptr<const Expression> StringOffset(
    unique_ptr<const Expression> const haystack,
    unique_ptr<const Expression> const needle);

// Returns true if needle appears in haystack.
unique_ptr<const Expression> StringContains(
    unique_ptr<const Expression> const haystack,
    unique_ptr<const Expression> const needle);

// Case insensitive variant of StringContains expression.
// The current implementation is not very efficient yet (uses conversion to
// lower string).
unique_ptr<const Expression> StringContainsCI(
    unique_ptr<const Expression> const haystack,
    unique_ptr<const Expression> const needle);

// Replace all occurences of "needle" in "haystack" with "substitute".
// Needle is treated as a string (no regexps).
unique_ptr<const Expression> StringReplace(
    unique_ptr<const Expression> haystack,
    unique_ptr<const Expression> needle,
    unique_ptr<const Expression> substitute);

}  // namespace supersonic

#endif  // SUPERSONIC_EXPRESSION_CORE_STRING_EXPRESSIONS_H_
