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
// Evaluators for hashing expressions.

#ifndef SUPERSONIC_EXPRESSION_EXT_HASHING_HASHING_EVALUATORS_H_
#define SUPERSONIC_EXPRESSION_EXT_HASHING_HASHING_EVALUATORS_H_

#include "supersonic/utils/integral_types.h"
#include "supersonic/utils/strings/stringpiece.h"
#include "supersonic/utils/hash/hash.h"

namespace supersonic {
namespace operators {

// The reason for the weird name is to avoid a collision with the hash.h
// fingerprint function.
struct FingerprintEvaluator {
  uint64_t operator()(uint64_t number) {
    return Fingerprint(number);
  }

  uint64_t operator()(StringPiece str) {
    return Fingerprint(str.data(), static_cast<uint32_t>(str.length()));
  }
};

// Here, in turn, we risk collision with our internal Hash struct in types.h.
struct HashEvaluator {
  uint64_t operator()(int32_t number, uint64_t seed) {
    return Hash64NumWithSeed(static_cast<uint64_t>(number), seed);
  }

  uint64_t operator()(uint32_t number, uint64_t seed) {
    return Hash64NumWithSeed(static_cast<uint64_t>(number), seed);
  }

  uint64_t operator()(int64_t number, uint64_t seed) {
    return Hash64NumWithSeed(number, seed);
  }

  uint64_t operator()(uint64_t number, uint64_t seed) {
    return Hash64NumWithSeed(number, seed);
  }

  uint64_t operator()(float number, uint64_t seed) {
    return Hash64FloatWithSeed(number, seed);
  }

  uint64_t operator()(double number, uint64_t seed) {
    return Hash64DoubleWithSeed(number, seed);
  }

  uint64_t operator()(StringPiece str, uint64_t seed) {
    return Hash64StringWithSeed(str.data(), str.length(), seed);
  }
};

}  // end namespace operators.
}  // end namespace supersonic.

#endif  // SUPERSONIC_EXPRESSION_EXT_HASHING_HASHING_EVALUATORS_H_
