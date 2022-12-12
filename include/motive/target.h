// Copyright 2015 Google Inc. All rights reserved.
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

#ifndef MOTIVE_TARGET_H_
#define MOTIVE_TARGET_H_

#include "motive/common.h"
#include "motive/math/range.h"
#include "motive/math/vector_converter.h"

namespace motive {

/// @class MotiveCurveShape
/// @brief A target curve shape for the motivator.
///
/// The curve shape is defined by the typical distance to travel, the time it
/// takes to travel it, and the bias. Using these variables, the actual time it
/// takes to travel the curve will be calculated.
struct MotiveCurveShape {
  MotiveCurveShape()
      : typical_delta_value(0.0f), typical_total_time(0.0f), bias(0.0f) {}
  MotiveCurveShape(float typical_delta_value, float typical_total_time,
                   float bias)
      : typical_delta_value(typical_delta_value),
        typical_total_time(typical_total_time),
        bias(bias) {}

  /// The typical difference between the start and end values.
  float typical_delta_value;

  /// The typical time it takes to go the typical distance.
  float typical_total_time;

  /// Determines how much the curve should ease-in and how much it should
  /// ease-out. Should be a value from 0.0 to 1.0.
  /// Examples of potential bias values and what they would represent:
  /// 0.0: ease-in but no ease out (a.k.a. "fly-out").
  /// 0.3: ease-in more slowly and ease-out more quickly (i.e. less responsive).
  /// 0.5: symmetrical curve: equal ease-in and ease-out.
  /// 0.7: ease-out more slowly and ease-in more quickly (i.e. more reponsive).
  /// 1.0: ease-out but no ease in (a.k.a. "fly-in").
  float bias;
};

/// @class MotiveNode1f
/// @brief A waypoint in MotiveTarget1f.
/// Describes one key point through which a value is animated.
struct MotiveNode1f {
  /// Desired value to animate to at `time`.
  float value;

  /// Speed when at `time`.
  float velocity;

  /// Time to achieve this key point.
  MotiveTime time;

  /// When using modular arithmetic, which of two directions to go.
  ModularDirection direction;

  MotiveNode1f()
      : value(0.0f),
        velocity(0.0f),
        time(0),
        direction(motive::kDirectionClosest) {}
  MotiveNode1f(float value, float velocity, MotiveTime time,
               motive::ModularDirection direction = motive::kDirectionClosest)
      : value(value), velocity(velocity), time(time), direction(direction) {}
};

/// @class MotiveTarget1f
/// @brief Set the current and/or target state for a one-dimensional Motivator.
///
/// A series of waypoints through which we animate. If the first waypoint has
/// time = 0, the current value and velocity jumps to that waypoint's value and
/// velocity.
///
/// MotiveTarget1fs are most easily created with the utility functions below,
/// for example Current1f, Target1f, CurrentToTarget1f.
///
/// If the current value and velocity are not specified (i.e. if the first
/// waypoint has time > 0), then the current value and velocity in
/// the Motivator are maintained.
///
/// If the target is not specified (i.e. only one waypoint which has time = 0),
/// then the current value is set as specified, and the velocity is set to 0.
///
class MotiveTarget1f {
 public:
  static const int kMaxNodes = 3;

  MotiveTarget1f() : num_nodes_(0) {}

  /// Create with only one waypoint.
  /// If n0.time = 0, set the current value and velocity.
  /// If n0.time > 0, maintain the current value and velocity and animate to
  /// n0's value and velocity in n0.time.
  explicit MotiveTarget1f(const MotiveNode1f& n0) : num_nodes_(1) {
    nodes_[0] = n0;
  }

  /// Create with two waypoints.
  /// Can be current to target, if n0.time = 0.
  /// Or can maintain the current and to through two targets: first n0, then n1.
  /// Precondition: n0.time < n1.time.
  MotiveTarget1f(const MotiveNode1f& n0, const MotiveNode1f& n1)
      : num_nodes_(2) {
    assert(0 <= n0.time && n0.time < n1.time);
    nodes_[0] = n0;
    nodes_[1] = n1;
  }

  /// Create with three waypoints.
  /// 0 <= n0.time < n1.time < n2.time
  MotiveTarget1f(const MotiveNode1f& n0, const MotiveNode1f& n1,
                 const MotiveNode1f& n2)
      : num_nodes_(3) {
    assert(0 <= n0.time && n0.time < n1.time && n1.time < n2.time);
    nodes_[0] = n0;
    nodes_[1] = n1;
    nodes_[2] = n2;
  }

  /// Empty the target of all waypoints.
  void Reset() { num_nodes_ = 0; }

  /// Return nth waypoint.
  /// @param node_index nth waypoint. 0 <= node_index < num_nodes()
  const MotiveNode1f& Node(int node_index) const {
    assert(0 <= node_index && node_index < num_nodes_);
    return nodes_[node_index];
  }

  /// Return smallest range that covers the values of all waypoints.
  /// @param start_value An extra value to include in the min/max calculation.
  ///                    Most often is the current value of the Motivator1f.
  motive::Range ValueRange(float start_value) const {
    assert(num_nodes_ > 0);
    float min = start_value;
    float max = start_value;
    for (int i = 0; i < num_nodes_; ++i) {
      min = std::min(nodes_[i].value, min);
      max = std::max(nodes_[i].value, max);
    }
    return motive::Range(min, max);
  }

  /// Return time of the last waypoint.
  MotiveTime EndTime() const {
    assert(num_nodes_ > 0);
    return nodes_[num_nodes_ - 1].time;
  }

  int num_nodes() const { return num_nodes_; }
  const MotiveTarget1f* targets() const { return this; }

 private:
  /// Length of nodes_.
  int num_nodes_;

  /// Constant-size array, to avoid dynamic memory allocation.
  /// This class is often used as a parameter and allocated on the stack.
  MotiveNode1f nodes_[kMaxNodes];
};

/// @class MotiveTargetN
/// @brief N-dimensional MotiveTargets are simply arrays of one dimensional
///        MotiveTargets.
template <int kDimensionsT>
class MotiveTargetN {
 public:
  static const int kDimensions = kDimensionsT;

  const MotiveTarget1f& operator[](int i) const {
    assert(0 <= i && i < kDimensions);
    return targets_[i];
  }

  MotiveTarget1f& operator[](int i) {
    assert(0 <= i && i < kDimensions);
    return targets_[i];
  }

  const MotiveTarget1f* targets() const { return targets_; }

 private:
  /// Each dimension gets its own target, that can be accessed with [].
  /// Note that the 'time' values do not have to correspond between dimensions.
  /// When using static constructor functions like `Current()` or `Target()`
  /// all the times will be the same, but in general, we want to allow each
  /// dimension to specify time independently. This will lead to maximal
  /// compression. There's no reason why the x, y, and z channels should act
  /// similarly.
  MotiveTarget1f targets_[kDimensions];
};

typedef MotiveTargetN<2> MotiveTarget2f;
typedef MotiveTargetN<3> MotiveTarget3f;
typedef MotiveTargetN<4> MotiveTarget4f;

template <int>
struct MotiveTargetT {
  typedef void type;
};
template <>
struct MotiveTargetT<1> {
  typedef MotiveTarget1f type;
};
template <>
struct MotiveTargetT<2> {
  typedef MotiveTarget2f type;
};
template <>
struct MotiveTargetT<3> {
  typedef MotiveTarget3f type;
};
template <>
struct MotiveTargetT<4> {
  typedef MotiveTarget4f type;
};

/// Set the Motivator's current values. Target values are reset to be the same
/// as the new current values.
inline MotiveTarget1f Current1f(float current_value,
                                float current_velocity = 0.0f) {
  return MotiveTarget1f(MotiveNode1f(current_value, current_velocity, 0));
}

/// Keep the Motivator's current values, but set the Motivator's target values.
/// If Motivator uses modular arithmetic, traverse from the current to the
/// target according to 'direction'.
inline MotiveTarget1f Target1f(float target_value, float target_velocity,
                               MotiveTime target_time,
                               motive::ModularDirection direction =
                                   motive::kDirectionClosest) {
  assert(target_time >= 0);
  return MotiveTarget1f(
      MotiveNode1f(target_value, target_velocity, target_time, direction));
}

/// Set both the current and target values for an Motivator.
inline MotiveTarget1f CurrentToTarget1f(
    float current_value, float current_velocity, float target_value,
    float target_velocity, MotiveTime target_time,
    motive::ModularDirection direction = motive::kDirectionClosest) {
  return MotiveTarget1f(
      MotiveNode1f(current_value, current_velocity, 0),
      MotiveNode1f(target_value, target_velocity, target_time, direction));
}

/// Move from the current value to the target value at a constant speed.
inline MotiveTarget1f CurrentToTargetConstVelocity1f(float current_value,
                                                     float target_value,
                                                     MotiveTime target_time) {
  assert(target_time > 0);
  const float velocity = (target_value - current_value) / target_time;
  return MotiveTarget1f(MotiveNode1f(current_value, velocity, 0),
                        MotiveNode1f(target_value, velocity, target_time,
                                     motive::kDirectionDirect));
}

/// Keep the Motivator's current values, but set two targets for the Motivator.
/// After the first target, go on to the next.
inline MotiveTarget1f TargetToTarget1f(float target_value,
                                       float target_velocity,
                                       MotiveTime target_time,
                                       float third_value, float third_velocity,
                                       MotiveTime third_time) {
  return MotiveTarget1f(
      MotiveNode1f(target_value, target_velocity, target_time),
      MotiveNode1f(third_value, third_velocity, third_time));
}

/// Set the Motivator's current values, and two targets afterwards.
inline MotiveTarget1f CurrentToTargetToTarget1f(
    float current_value, float current_velocity, float target_value,
    float target_velocity, MotiveTime target_time, float third_value,
    float third_velocity, MotiveTime third_time) {
  return MotiveTarget1f(
      MotiveNode1f(current_value, current_velocity, 0),
      MotiveNode1f(target_value, target_velocity, target_time),
      MotiveNode1f(third_value, third_velocity, third_time));
}

/// Utility functions to construct MotiveTargets of dimension >= 2.
template <class VectorConverter, int kDimensionsT>
class MotiveTargetBuilderTemplate {
  typedef VectorConverter C;

 public:
  static const int kDimensions = kDimensionsT;
  typedef typename VectorT<C, kDimensionsT>::type Vec;
  typedef MotiveTargetN<kDimensionsT> MotiveTarget;
  typedef const Vec& VecIn;

  /// Set the Motivator's current values. Target values are reset to be the same
  /// as the new current values.
  static MotiveTarget Current(VecIn current_value,
                              VecIn current_velocity = Vec(0.0f)) {
    const float* current_value_in = C::ToPtr(current_value);
    const float* current_velocity_in = C::ToPtr(current_velocity);

    MotiveTarget t;
    for (int i = 0; i < kDimensions; ++i) {
      t[i] = Current1f(current_value_in[i], current_velocity_in[i]);
    }
    return t;
  }

  /// Keep the Motivator's current values, but set the Motivator's target
  /// values. If Motivator uses modular arithmetic, traverse from the current
  /// to the target according to 'direction'.
  static MotiveTarget Target(
      VecIn target_value, VecIn target_velocity, MotiveTime target_time,
      motive::ModularDirection direction = motive::kDirectionClosest) {
    const float* target_value_in = C::ToPtr(target_value);
    const float* target_velocity_in = C::ToPtr(target_velocity);

    MotiveTarget t;
    for (int i = 0; i < kDimensions; ++i) {
      t[i] = Target1f(target_value_in[i], target_velocity_in[i], target_time,
                      direction);
    }
    return t;
  }

  /// Set both the current and target values for an Motivator.
  static MotiveTarget CurrentToTarget(
      VecIn current_value, VecIn current_velocity, VecIn target_value,
      VecIn target_velocity, MotiveTime target_time,
      motive::ModularDirection direction = motive::kDirectionClosest) {
    const float* current_value_in = C::ToPtr(current_value);
    const float* current_velocity_in = C::ToPtr(current_velocity);
    const float* target_value_in = C::ToPtr(target_value);
    const float* target_velocity_in = C::ToPtr(target_velocity);

    MotiveTarget t;
    for (int i = 0; i < kDimensions; ++i) {
      t[i] = CurrentToTarget1f(current_value_in[i], current_velocity_in[i],
                               target_value_in[i], target_velocity_in[i],
                               target_time, direction);
    }
    return t;
  }

  /// Move from the current value to the target value at a constant speed.
  static MotiveTarget CurrentToTargetConstVelocity(VecIn current_value,
                                                   VecIn target_value,
                                                   MotiveTime target_time) {
    const float* current_value_in = C::ToPtr(current_value);
    const float* target_value_in = C::ToPtr(target_value);

    // TODO OPT: Do the math using the vector types.
    MotiveTarget t;
    for (int i = 0; i < kDimensions; ++i) {
      t[i] = CurrentToTargetConstVelocity1f(current_value_in[i],
                                            target_value_in[i], target_time);
    }
    return t;
  }
};

/// Utility functions to construct MotiveTarget1fs.
template <class VectorConverter>
class MotiveTargetBuilderTemplate<VectorConverter, 1> {
 public:
  static const int kDimensions = 1;
  typedef float Vec;
  typedef MotiveTarget1f MotiveTarget;
  typedef float VecIn;

  static MotiveTarget Current(VecIn current_value,
                              VecIn current_velocity = Vec(0.0f)) {
    return Current1f(current_value, current_velocity);
  }

  static MotiveTarget Target(
      VecIn target_value, VecIn target_velocity, MotiveTime target_time,
      motive::ModularDirection direction = motive::kDirectionClosest) {
    return Target1f(target_value, target_velocity, target_time, direction);
  }

  static MotiveTarget CurrentToTarget(
      VecIn current_value, VecIn current_velocity, VecIn target_value,
      VecIn target_velocity, MotiveTime target_time,
      motive::ModularDirection direction = motive::kDirectionClosest) {
    return CurrentToTarget1f(current_value, current_velocity, target_value,
                             target_velocity, target_time, direction);
  }

  static MotiveTarget CurrentToTargetConstVelocity(VecIn current_value,
                                                   VecIn target_value,
                                                   MotiveTime target_time) {
    return CurrentToTargetConstVelocity1f(current_value, target_value,
                                          target_time);
  }
};

// Typedefs to make it easy to construct MotiveTargets with dimension >= 2.
//
// For example, to create a MotiveTarget2f that sets both the current and
// future values, you could use this syntax.
//
//     Mo2f::CurrentToTarget(vec2(0,1), vec2(0,0), vec2(2,3), vec2(1,1), 100);
//
// These classes use mathfu types in their external API. If you have your own
// vector types, create your own VectorConverter and your own typedefs.
//
typedef MotiveTargetBuilderTemplate<motive::MathFuVectorConverter, 1> Tar1f;
typedef MotiveTargetBuilderTemplate<motive::MathFuVectorConverter, 2> Tar2f;
typedef MotiveTargetBuilderTemplate<motive::MathFuVectorConverter, 3> Tar3f;
typedef MotiveTargetBuilderTemplate<motive::MathFuVectorConverter, 4> Tar4f;

}  // namespace motive

#endif  // MOTIVE_TARGET_H
