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

#ifndef MOTIVE_RIG_ANIM_H_
#define MOTIVE_RIG_ANIM_H_

#include <vector>
#include "motive/math/compact_spline.h"
#include "motive/matrix_anim.h"

namespace motive {

/// @class RigAnim
/// @brief Animation for a RigMotivator. Drives a fully rigged model.
class RigAnim {
 public:
  RigAnim() : end_time_(0), repeat_(false) {}

  /// Initialize the basic data. After calling this function, `InitMatrixAnim()`
  /// should be called once for every bone in the animation.
  void Init(const char* anim_name, BoneIndex num_bones, bool record_names);

  /// For construction. Return the `idx`th bone's animation for initialization.
  /// @param idx The bone whose animation you want to initialize.
  /// @param parent If no parent exists, pass in kInvalidBoneIdx.
  /// @param bone_name For debugging. Recorded if `record_names` was true in
  ///                  `Init()`.
  MatrixAnim& InitMatrixAnim(BoneIndex idx, BoneIndex parent,
                             const char* bone_name);

  /// Return the animation of the `idx`th bone. Each bone animates a matrix.
  const MatrixAnim& Anim(BoneIndex idx) const {
    assert(idx < anims_.size());
    return anims_[idx];
  }

  /// Number of bones. Bones are arranged in an hierarchy. Each bone animates
  /// a matrix. The matrix describes the transform of the bone from its parent.
  BoneIndex NumBones() const { return static_cast<BoneIndex>(anims_.size()); }

  /// For debugging. If `record_names` was specified in `Init()`, the names of
  /// the bones are stored. Very useful when an animation is applied to a mesh
  /// that doesn't match: with the bone names you can determine whether the
  /// mesh or the animation is out of date.
  const char* BoneName(BoneIndex idx) const {
    return idx < bone_names_.size() ? bone_names_[idx].c_str() : "unknown";
  }

  /// Total number of matrix operations across all MatrixAnims in this RigAnim.
  int NumOps() const;

  /// Gets the splines and constants that drive the operations in `ops`,
  /// for the specified bone. If an operation is not driven by the bone,
  /// return the default value for that op in `constants`.
  ///
  /// If the bone has multiple operations that match `ops[i]`, return the
  /// first one.
  ///
  /// @param bone The bone whose operations you want to pull data for.
  /// @param ops And array of length `num_ops` with the operations you're
  ///            interested in.
  /// @params num_ops Length of the `ops` array.
  /// @params splines Output array, length `num_ops`. For each element of
  ///                 `ops`, receives the driving spline, or nullptr if that
  ///                 operation is not driven by a spline.
  /// @params constants Output array, length `num_ops`. For each element of
  ///                 `ops`, receives the constant value of that operation,
  ///                 if no spline drives that operation.
  void GetSplinesAndConstants(BoneIndex bone, const MatrixOperationType* ops,
                              int num_ops, const CompactSpline** splines,
                              float* constants) const;

  /// For debugging. The number of lines in the header. You call them separately
  /// in case you want to prefix or append extra columns.
  int NumCsvHeaderLines() const { return 2; }

  /// Output a line of comma-separated-values that has header information for
  /// the CSV data output by RigMotivator::CsvValues().
  std::string CsvHeaderForDebugging(int line) const;

  /// Amount of time required by this animation. Time units are set by the
  /// caller. If animation repeats, returns infinity.
  /// TODO: Add function to return non-repeated end time, even when repeatable.
  MotiveTime end_time() const { return end_time_; }

  /// For construction. The end time should be set to the maximal end time of
  /// all the `anims_`.
  void set_end_time(MotiveTime t) { end_time_ = t; }

  /// Returns an array of length NumBones() representing the bone heirarchy.
  /// `bone_parents()[i]` is the bone index of the ith bone's parent.
  /// `bone_parents()[i]` < `bone_parents()[j]` for all i < j.
  /// For bones at the root (i.e. no parent) value is kInvalidBoneIdx.
  const BoneIndex* bone_parents() const { return bone_parents_.data(); }

  /// Animation is repeatable. That is, when the end of the animation is
  /// reached, it can be started at the beginning again without glitching.
  /// Generally, an animation is repeatable if it's curves have the same values
  /// and derivatives at the start and end.
  bool repeat() const { return repeat_; }

  /// Set the repeat flag.
  bool set_repeat(bool repeat) { return repeat_ = repeat; }

  /// For debugging. The name of the animation currently being played.
  /// Only valid if `record_names` is true in `Init()`.
  const std::string& anim_name() const { return anim_name_; }

 private:
  std::vector<MatrixAnim> anims_;
  std::vector<BoneIndex> bone_parents_;
  std::vector<std::string> bone_names_;
  MotiveTime end_time_;
  bool repeat_;
  std::string anim_name_;
};

}  // namespace motive

#endif  // MOTIVE_RIG_ANIM_H_
