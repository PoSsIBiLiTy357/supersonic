// Copyright 2009 Google Inc. All Rights Reserved.
// Based on code by austern@google.com (Matt Austern)
// Copyright 2017 Adfin Solutions Inc. All Rights Reserved.


#ifndef UTIL_GTL_POINTER_VECTOR_H_
#define UTIL_GTL_POINTER_VECTOR_H_

#include <vector>
#include <memory>


namespace util {
namespace gtl {

// Container class PointerVector<T> is modeled after the interface
// of vector<std::unique_ptr<T> >, with the important distinction that
// it compiles.  It has the same performance characteristics:
// O(1) access to the nth element, O(N) insertion in the middle, and
// amortized O(1) insertion at the end. See clause 23 of the C++ standard.
// And base/scoped_ptr.h, of course.
//
// Exceptions are:
//
// 1) You can not construct a PointerVector of multiple elements unless they're
//    all NULL, nor can you insert() multiple elements unless the pointer being
//    inserted is NULL.
// 1a) This means the iteration constructor and copy consructor are not
//     supported.
//
// 2) assignment is not supported.
//
// 3) The iterator form of insert is not supported.
//
// 4) resize() only ever fills with NULL.
//
// 5) You can't use relational operators to compare 2 PointerVectors
//

// Replaces the old PointerVector
// TOOD: Replace this std::vector<std::unique_ptr> everywhere
template <typename T>
using PointerVector = std::vector<std::unique_ptr<T>>;


}  // namespace gtl
}  // namespace util

#endif  // UTIL_GTL_POINTER_VECTOR_H_
