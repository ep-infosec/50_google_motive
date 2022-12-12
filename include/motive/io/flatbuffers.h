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

#ifndef MOTIVE_IO_FLATBUFFERS_H_
#define MOTIVE_IO_FLATBUFFERS_H_

namespace motive {

class AnimTable;
class MatrixAnim;
struct MatrixAnimFb;
class OvershootInit;
struct OvershootParameters;
class RigAnim;
struct RigAnimFb;
class SplineInit;
struct SplineParameters;
struct Settled1f;
struct Settled1fParameters;

/// Convert from FlatBuffer params to Motive init, for Overshoot.
void OvershootInitFromFlatBuffers(const OvershootParameters& params,
                                  OvershootInit* init);

/// Convert from FlatBuffer params to Motive init, for Spline.
void SplineInitFromFlatBuffers(const SplineParameters& params,
                               SplineInit* init);

/// Convert from FlatBuffer params to Motive Settled1f.
void Settled1fFromFlatBuffers(const Settled1fParameters& params,
                              Settled1f* settled);

/// Convert from FlatBuffer params to Motive MatrixAnim.
void MatrixAnimFromFlatBuffers(const MatrixAnimFb& params, MatrixAnim* anim);

/// Convert from FlatBuffer params to Motive MatrixAnim.
void RigAnimFromFlatBuffers(const RigAnimFb& params, RigAnim* anim);

}  // namespace motive

#endif  // MOTIVE_IO_FLATBUFFERS_H_
