// Copyright 2011 Google Inc.  All Rights Reserved
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
// Defines a helper 'smart pointer' class used in exception propagation. It's
// similar to linked_ptr, but w/ different release semantics: releasing does
// not set the value to NULL, and does not require that the argument is the
// last member of the linked list. Calling release() on any co-owner revokes
// ownership from all of them, transferring it to the caller. Nonetheless, after
// the release, get() continues to return the original pointer. (It is the
// responsibility of the caller of get() to ensure that the object, if
// it has been released, is still alive).

#ifndef DATAWAREHOUSE_COMMON_EXCEPTION_COOWNED_POINTER_H_
#define DATAWAREHOUSE_COMMON_EXCEPTION_COOWNED_POINTER_H_

#include <atomic>

#include <glog/logging.h>
#include "supersonic/utils/logging-inl.h"  // for CHECK macros

namespace common {

template<typename T>
class CoownedPointer {
 public:
  // Creates a pointer to NULL.
  explicit CoownedPointer() {}

  // If the value is not NULL, creates a pointer that is a sole owner of the
  // value. Otherwise, creates a pointer to NULL.
  explicit CoownedPointer(T* value)
      : value_(value),
        control_(new Control(value)) { }

  // Makes this a copy of the other.
  // If other.get() == NULL, creates a pointer to NULL. Otherwise, creates a
  // pointer to the other's value, that will be a co-owner iff the other is
  // an owner (i.e. iff the value has not been released).
  CoownedPointer(const CoownedPointer& other)
    : value_(other.value_)
  {
    if (other.control_) {
      control_ = other.control_;
      control_->count++;
    }
  }

  CoownedPointer(CoownedPointer&& other)  
      : value_(other.value_),
        control_(other.control_)
  {
    other.value_ = nullptr;
    other.control_ = nullptr;
  }

  ~CoownedPointer()
  { 
    if (control_ == nullptr)
      return;

    if (control_->count.fetch_sub(1) == 1) {
      delete control_->value.load();
      delete control_;
    }

  }

  // Returns a pointer to the value, without transferring ownership.
  // Continues to return the value even after release(). If !this->is_owner(),
  // it is the responsibility of the caller to ensure that the value is alive.
  T* get() { return value_; }

  // Returns a const pointer to the value, without transferring ownership.
  // Continues to return the value even after release(). If !this->is_owner(),
  // it is the responsibility of the caller to ensure that the value is alive.
  const T* get() const { return value_; }

  // Makes this point to the other's object, and co-own it if it has not been
  // released.
  CoownedPointer& operator=(const CoownedPointer& other)
  {
    if (&other != this) {
      value_ = other.value_;
      control_ = other.control_;
      control_->count++;
    }

    return *this;
  }

  CoownedPointer& operator=(CoownedPointer&& other)
  {
    if (&other != this) {
      std::swap(value_, other.value_);
      std::swap(control_, other.control_);
    }

    return *this;
  }

  // Returns true if the referenced object is not NULL and has not yet been
  // released. If (!is_owner() && get() != NULL), then calling release() is
  // prohibited (as the object has already been released).
  bool is_owner() const 
  {
    if (control_ == nullptr)
      return false;

    return control_->value.load() != nullptr;
  }

  // If the referenced object is NULL, this method has no effect and returns
  // NULL. Otherwise, it returns the referenced object, passing its ownership
  // to the caller.
  // After this method is called with a non-NULL referenced object:
  //   * is_owner() will return false for this and all peers.
  //   * get() will continue to return the referenced object.
  //   * deleting this and peers will not destroy the referenced object.
  //   * calling release() again, on this on any peers, is illegal (will cause
  //     crash).
  T* release() {
    if (control_ == nullptr) {
      CHECK(value_ == nullptr) << "Release called on a released pointer.";
    } else {
      leave();
    }

    return value_;
  }

 private:
  struct Control {
    std::atomic<int> count;
    std::atomic<T*> value;

    Control(T* value)
      : count(1),
        value(value) {}
  };

  void leave()
  {
    if (control_ == nullptr)
      return;

    control_->value.store(nullptr);

    if (control_->count.fetch_sub(1) == 1)
      delete control_;

    control_ = nullptr;
  }

 private:
  // Pointer to the referenced object.
  T* value_ = nullptr;
  // Shared control block
  Control* control_ = nullptr;
};

}  // namespace

#endif  // DATAWAREHOUSE_COMMON_EXCEPTION_COOWNED_POINTER_H_
