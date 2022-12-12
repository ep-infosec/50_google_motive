// Copyright 2014 Google Inc. All rights reserved.
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

#ifndef MOTIVE_UTIL_H_
#define MOTIVE_UTIL_H_

#ifdef _MSC_VER
#include <malloc.h>
#endif
#include <stddef.h>
#include <stdlib.h>
#include "motive/motivator.h"
#include "motive/target.h"
#include "motive/vector_motivator.h"

namespace motive {

// Allocates a block of memory of the given size and alignment.  This memory
// must be freed by calling AlignedFree.
inline void* AlignedAlloc(size_t size, size_t align) {
  const size_t min_align = std::max(align, sizeof(max_align_t));
#ifdef _MSC_VER
  return _aligned_malloc(size, min_align);
#else
  void* ptr = nullptr;
  posix_memalign(&ptr, min_align, size);
  return ptr;
#endif
}

// Frees memory allocated using AlignedAlloc.
inline void AlignedFree(void* ptr) {
#ifdef _MSC_VER
  _aligned_free(ptr);
#else
  free(ptr);
#endif
}

/// Direction to boost the value.
enum TwitchDirection {
  kTwitchDirectionNone,      /// Do nothing.
  kTwitchDirectionPositive,  /// Give the velocity a positive boost.
  kTwitchDirectionNegative   /// Give the velocity a negative boost.
};

/// @class Settled1f
/// @brief Helper to determine if we're "at the target" and "stopped".
struct Settled1f {
  Settled1f() : max_difference(0.0f), max_velocity(0.0f) {}

  /// Return true if our distance from target is and velocity are less than
  /// this class' threshold.
  bool Settled(float dist, float velocity) const {
    return fabs(dist) <= max_difference && fabs(velocity) <= max_velocity;
  }

  /// Return true if `motivator` is "at the target" and "stopped".
  template <class Motivator>
  bool Settled(const Motivator& motivator) const {
    float differences[Motivator::kDimensions];
    float velocities[Motivator::kDimensions];
    motivator.Differences(differences);
    motivator.Velocities(velocities);
    for (int i = 0; i < Motivator::kDimensions; ++i) {
      if (!Settled(differences[i], velocities[i])) return false;
    }
    return true;
  }

  /// Consider ourselves "at the target" if the absolute difference between
  /// the value and the target is less than this.
  float max_difference;

  /// Consider ourselves "stopped" if the absolute velocity is less than this.
  float max_velocity;
};

template <>
inline bool Settled1f::Settled<Motivator1f>(const Motivator1f& motivator)
    const {
  return Settled(motivator.Difference(), motivator.Velocity());
}

/// If `motivator` is "at the target" and "stopped" give it a boost in
/// `direction`.
///
/// A little boost is useful to demonstrate responsiveness to user input,
/// even when you can't logically change to a new state. A slight boost that
/// then settles back to its original value (via an OvershootMotivator, for
/// example) looks and feels correct.
inline void Twitch(TwitchDirection direction, float velocity,
                   const Settled1f& settled, Motivator1f* motivator) {
  if (direction != kTwitchDirectionNone && settled.Settled(*motivator)) {
    motivator->SetTarget(Current1f(
        motivator->Value(),
        direction == kTwitchDirectionPositive ? velocity : -velocity));
  }
}

}  // namespace motive

#endif  // MOTIVE_UTIL_H_
