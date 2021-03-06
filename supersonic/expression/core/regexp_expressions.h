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

#ifndef SUPERSONIC_EXPRESSION_CORE_REGEXP_EXPRESSIONS_H_
#define SUPERSONIC_EXPRESSION_CORE_REGEXP_EXPRESSIONS_H_

#include "supersonic/utils/strings/stringpiece.h"
#include "supersonic/utils/std_namespace.h"
#include "supersonic/expression/base/expression.h"

namespace supersonic {

// Performs partial regular expression matching, using RE2, on the specified
// string argument. Returns true if matched, false if not matched, NULL if
// the argument is NULL.
//
// Note: the argument order contravenes the standard SuperSonic order of
// "variable arguments at the end".
unique_ptr<const Expression> RegexpPartialMatch(unique_ptr<const Expression> str,
                                                const StringPiece& pattern);

// Performs full regular expression matching, using RE2, on the specified
// string argument. Returns true if matched, false if not matched, NULL if
// the argument is NULL.
//
// Note: the argument order contravenes the standard SuperSonic order of
// "variable arguments at the end".
unique_ptr<const Expression> RegexpFullMatch(unique_ptr<const Expression> str,
                                             const StringPiece& pattern);

// Replace all occurences of "needle" in "haystack" with "substitute".
// Needle can be a regular expression.
unique_ptr<const Expression> RegexpReplace(unique_ptr<const Expression> haystack,
                                           const StringPiece& needle,
                                           unique_ptr<const Expression> substitute);

// Replace the first match of "pattern" in "str" with "rewrite". Within
// "rewrite", backslash-escaped digits (\1 to \9) can be used to insert text
// matching corresponding parenthesized group from the pattern.  \0 in
// "rewrite" refers to the entire matching text.
// If not matched, or if the argument is NULL, results in NULL.
//
// Currently not implemented.
unique_ptr<const Expression> RegexpRewrite(unique_ptr<const Expression> str,
                                           const StringPiece& pattern,
                                           const StringPiece& rewrite);

// Return the first substring of "str" matching "pattern". If "pattern" cannot
// be matched into substring, returns NULL.
unique_ptr<const Expression> RegexpExtract(unique_ptr<const Expression> str,
                                           const StringPiece& pattern);

// Replace the first match of "pattern" in "str" with "rewrite". Within
// "rewrite", backslash-escaped digits (\1 to \9) can be used to insert text
// matching corresponding parenthesized group from the pattern.  \0 in
// "rewrite" refers to the entire matching text.
// If the argument is NULL, results in NULL. If the argument is not NULL but
// the pattern did not match, returns the default value.
//
// Currently not implemented.
unique_ptr<const Expression> RegexpRewrite(
    unique_ptr<const Expression> str,
    unique_ptr<const Expression> default_value,
    const StringPiece& pattern,
    const StringPiece& rewrite);

}  // namespace supersonic

#endif  // SUPERSONIC_EXPRESSION_CORE_REGEXP_EXPRESSIONS_H_
