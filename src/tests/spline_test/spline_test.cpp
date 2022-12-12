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

#include "gtest/gtest.h"
#include "motive/common.h"
#include "motive/math/angle.h"
#include "motive/math/bulk_spline_evaluator.h"
#include "motive/math/compact_spline.h"

using motive::QuadraticCurve;
using motive::CubicCurve;
using motive::CubicInit;
using motive::Range;
using motive::CompactSpline;
using motive::CompactSplineIndex;
using motive::BulkSplineEvaluator;
using motive::Angle;
using motive::kPi;
using mathfu::vec2;
using mathfu::vec2i;
using mathfu::vec3;
using mathfu::vec3_packed;

// Print the curves in a format that can be cut-and-paste into a spreadsheet.
// Working in a spreadsheet is nice because of the graphing features.
#define PRINT_SPLINES_AS_CSV 0

// Draw an ASCII graph of the curves. Helpful for a quick visualization, though
// not very high fidelity, obviously.
#define PRINT_SPLINES_AS_ASCII_GRAPHS 1

struct GraphDerivatives {
  float first;
  float second;
  float third;
  GraphDerivatives() : first(0.0f), second(0.0f), third(0.0f) {}
  GraphDerivatives(float first, float second, float third)
      : first(first), second(second), third(third) {}
};

struct GraphData {
  std::vector<vec2> points;
  std::vector<GraphDerivatives> derivatives;
};

static const int kNumCheckPoints = motive::kDefaultGraphWidth;
static const vec2i kGraphSize(kNumCheckPoints, motive::kDefaultGraphHeight);
static const float kFixedPointEpsilon = 0.02f;
static const float kDerivativePrecision = 0.01f;
static const float kSecondDerivativePrecision = 0.26f;
static const float kThirdDerivativePrecision = 6.0f;
static const float kNodeXPrecision = 0.0001f;
static const float kNodeYPrecision = 0.0001f;
static const float kXGranularityScale = 0.01f;
static const Range kAngleRange(-kPi, kPi);

// Use a ridiculous index that will never hit when doing a search.
// We use this to test the binary search algorithm, not the cache.
static const CompactSplineIndex kRidiculousSplineIndex = 10000;

static const CubicInit kSimpleSplines[] = {
    //    start_y         end_y        width_x
    //      start_derivative  end_derivative
    CubicInit(0.0f, 1.0f, 0.1f, 0.0f, 1.0f),
    CubicInit(1.0f, -8.0f, 0.0f, 0.0f, 1.0f),
    CubicInit(1.0f, -8.0f, -1.0f, 0.0f, 1.0f),
};
static const int kNumSimpleSplines =
    static_cast<int>(MOTIVE_ARRAY_SIZE(kSimpleSplines));

static const CubicInit CubicInitMirrorY(const CubicInit& init) {
  return CubicInit(-init.start_y, -init.start_derivative, -init.end_y,
                   -init.end_derivative, init.width_x);
}

static const CubicInit CubicInitScaleX(const CubicInit& init, float scale) {
  return CubicInit(init.start_y, init.start_derivative / scale, init.end_y,
                   init.end_derivative / scale, init.width_x * scale);
}

static Range CubicInitYRange(const CubicInit& init, float buffer_percent) {
  return motive::CreateValidRange(init.start_y, init.end_y)
      .Lengthen(buffer_percent);
}

static void InitializeSpline(const CubicInit& init, CompactSpline* spline) {
  const Range y_range = CubicInitYRange(init, 0.1f);
  spline->Init(y_range, init.width_x * kXGranularityScale);
  spline->AddNode(0.0f, init.start_y, init.start_derivative);
  spline->AddNode(init.width_x, init.end_y, init.end_derivative);
}

static void ExecuteInterpolator(BulkSplineEvaluator& interpolator,
                                int num_points, GraphData* d) {
  const float y_precision =
      interpolator.SourceSpline(0)->RangeY().Length() * kFixedPointEpsilon;

  const Range range_x = interpolator.SourceSpline(0)->RangeX();
  const float delta_x = range_x.Length() / (num_points - 1);

  for (int i = 0; i < num_points; ++i) {
    const CubicCurve& c = interpolator.Cubic(0);
    const float x = interpolator.CubicX(0);

    EXPECT_NEAR(c.Evaluate(x), interpolator.Y(0), y_precision);
    EXPECT_NEAR(c.Derivative(x), interpolator.Derivative(0),
                kDerivativePrecision);

    const vec2 point(interpolator.X(0), interpolator.Y(0));
    const GraphDerivatives derivatives(interpolator.Derivative(0),
                                       c.SecondDerivative(x),
                                       c.ThirdDerivative(x));
    d->points.push_back(point);
    d->derivatives.push_back(derivatives);

    interpolator.AdvanceFrame(delta_x);
  }
}

static void PrintGraphDataAsCsv(const GraphData& d) {
  (void)d;
#if PRINT_SPLINES_AS_CSV
  for (size_t i = 0; i < d.points.size(); ++i) {
    printf("%f, %f, %f, %f, %f\n", d.points[i].x, d.points[i].y,
           d.derivatives[i].first, d.derivatives[i].second,
           d.derivatives[i].third);
  }
#endif  // PRINT_SPLINES_AS_CSV
}

static void PrintSplineAsAsciiGraph(const GraphData& d) {
  (void)d;
#if PRINT_SPLINES_AS_ASCII_GRAPHS
  printf("\n%s\n\n",
         motive::Graph2DPoints(&d.points[0], static_cast<int>(d.points.size()),
                               kGraphSize)
             .c_str());
#endif  // PRINT_SPLINES_AS_ASCII_GRAPHS
}

static void GatherGraphData(
    const CubicInit& init, GraphData* d, bool is_angle = false,
    const motive::SplinePlayback& playback = motive::SplinePlayback()) {
  CompactSpline spline;
  InitializeSpline(init, &spline);

  BulkSplineEvaluator interpolator;
  interpolator.SetNumIndices(1);
  if (is_angle) {
    interpolator.SetYRanges(0, 1, kAngleRange);
  }
  interpolator.SetSplines(0, 1, &spline, playback);

  ExecuteInterpolator(interpolator, kNumCheckPoints, d);

  // Double-check start and end y values and derivitives, taking y-scale and
  // y-offset into account.
  const CubicCurve c(init);
  const float y_precision = spline.RangeY().Length() * kFixedPointEpsilon;
  const float derivative_precision =
      fabs(playback.y_scale) * kDerivativePrecision;
  EXPECT_NEAR(c.Evaluate(0.0f) * playback.y_scale + playback.y_offset,
              d->points[0].y, y_precision);
  EXPECT_NEAR(c.Derivative(0.0f) * playback.y_scale, d->derivatives[0].first,
              derivative_precision);
  EXPECT_NEAR(c.Evaluate(init.width_x) * playback.y_scale + playback.y_offset,
              d->points[kNumCheckPoints - 1].y, y_precision);
  EXPECT_NEAR(c.Derivative(init.width_x) * playback.y_scale,
              d->derivatives[kNumCheckPoints - 1].first, derivative_precision);

  PrintGraphDataAsCsv(*d);
  PrintSplineAsAsciiGraph(*d);
}

class SplineTests : public ::testing::Test {
 protected:
  virtual void SetUp() {
    short_spline_.Init(Range(0.0f, 1.0f), 0.01f);
    short_spline_.AddNode(0.0f, 0.1f, 0.0f, motive::kAddWithoutModification);
    short_spline_.AddNode(1.0f, 0.4f, 0.0f, motive::kAddWithoutModification);
    short_spline_.AddNode(4.0f, 0.2f, 0.0f, motive::kAddWithoutModification);
    short_spline_.AddNode(40.0f, 0.2f, 0.0f, motive::kAddWithoutModification);
    short_spline_.AddNode(100.0f, 1.0f, 0.0f, motive::kAddWithoutModification);
  }
  virtual void TearDown() {}

  CompactSpline short_spline_;
};

// Test in-place creation and destruction.
TEST_F(SplineTests, InPlaceCreation) {
  // Create a buffer with a constant fill.
  static const uint8_t kTestFill = 0xAB;
  uint8_t buffer[1024];
  memset(buffer, kTestFill, sizeof(buffer));

  // Dynamically create a spline in the buffer.
  static const int kTestMaxNodes = 3;
  static const size_t kSplineSize = CompactSpline::Size(kTestMaxNodes);
  assert(kSplineSize < sizeof(buffer));  // Strictly less to test for overflow.
  CompactSpline* spline = CompactSpline::CreateInPlace(kTestMaxNodes, buffer);
  EXPECT_EQ(kTestMaxNodes, spline->max_nodes());
  EXPECT_EQ(0, spline->num_nodes());

  // Create spline and ensure it now has the max size.
  spline->Init(kAngleRange, 1.0f);
  for (int i = 0; i < kTestMaxNodes; ++i) {
    spline->AddNode(static_cast<float>(i), 0.0f, 0.0f,
                    motive::kAddWithoutModification);
  }
  EXPECT_EQ(kTestMaxNodes, spline->max_nodes());
  EXPECT_EQ(kTestMaxNodes, spline->num_nodes());

  // Ensure the spline hasn't overflowed its buffer.
  for (size_t j = kSplineSize; j < sizeof(buffer); ++j) {
    EXPECT_EQ(buffer[j], kTestFill);
  }

  // Test node destruction.
  spline->~CompactSpline();
}

// Ensure the index lookup is accurate for x's before the range.
TEST_F(SplineTests, IndexForXBefore) {
  EXPECT_EQ(motive::kBeforeSplineIndex,
            short_spline_.IndexForX(-1.0f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x's barely before the range.
TEST_F(SplineTests, IndexForXJustBefore) {
  EXPECT_EQ(0, short_spline_.IndexForX(-0.0001f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x's barely before the range.
TEST_F(SplineTests, IndexForXBiggerThanGranularityAtStart) {
  EXPECT_EQ(0, short_spline_.IndexForX(-0.011f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x's after the range.
TEST_F(SplineTests, IndexForXAfter) {
  EXPECT_EQ(motive::kAfterSplineIndex,
            short_spline_.IndexForX(101.0f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x's barely after the range.
TEST_F(SplineTests, IndexForXJustAfter) {
  EXPECT_EQ(short_spline_.LastSegmentIndex(),
            short_spline_.IndexForX(100.0001f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x right at start.
TEST_F(SplineTests, IndexForXStart) {
  EXPECT_EQ(0, short_spline_.IndexForX(0.0f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x right at end.
TEST_F(SplineTests, IndexForXEnd) {
  EXPECT_EQ(short_spline_.LastSegmentIndex(),
            short_spline_.IndexForX(100.0f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x just inside end.
TEST_F(SplineTests, IndexForXAlmostEnd) {
  EXPECT_EQ(short_spline_.LastSegmentIndex(),
            short_spline_.IndexForX(99.9999f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x just inside end.
TEST_F(SplineTests, IndexForXBiggerThanGranularityAtEnd) {
  EXPECT_EQ(3, short_spline_.IndexForX(99.99f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x in middle, right on the node.
TEST_F(SplineTests, IndexForXMidOnNode) {
  EXPECT_EQ(1, short_spline_.IndexForX(1.0f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x in middle, in middle of segment.
TEST_F(SplineTests, IndexForXMidAfterNode) {
  EXPECT_EQ(1, short_spline_.IndexForX(1.1f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x in middle, in middle of segment.
TEST_F(SplineTests, IndexForXMidSecondLast) {
  EXPECT_EQ(2, short_spline_.IndexForX(4.1f, kRidiculousSplineIndex));
}

// Ensure the splines don't overshoot their mark.
TEST_F(SplineTests, Overshoot) {
  for (int i = 0; i < kNumSimpleSplines; ++i) {
    const CubicInit& init = kSimpleSplines[i];

    GraphData d;
    GatherGraphData(init, &d);

    const Range x_range(-kXGranularityScale,
                        init.width_x * (1.0f + kXGranularityScale));
    const Range y_range = CubicInitYRange(init, 0.001f);
    for (size_t j = 0; j < d.points.size(); ++j) {
      EXPECT_TRUE(x_range.Contains(d.points[j].x));
      EXPECT_TRUE(y_range.Contains(d.points[j].y));
    }
  }
}

// Ensure that the curves are mirrored in y when node y's are mirrored.
TEST_F(SplineTests, MirrorY) {
  for (int i = 0; i < kNumSimpleSplines; ++i) {
    const CubicInit& init = kSimpleSplines[i];
    const CubicInit mirrored_init = CubicInitMirrorY(init);
    const float y_precision =
        fabs(init.start_y - init.end_y) * kFixedPointEpsilon;

    GraphData d, mirrored_d;
    GatherGraphData(init, &d);
    GatherGraphData(mirrored_init, &mirrored_d);

    EXPECT_EQ(d.points.size(), mirrored_d.points.size());
    const int num_points = static_cast<int>(d.points.size());
    for (int j = 0; j < num_points; ++j) {
      EXPECT_EQ(d.points[j].x, mirrored_d.points[j].x);
      EXPECT_NEAR(d.points[j].y, -mirrored_d.points[j].y, y_precision);
      EXPECT_NEAR(d.derivatives[j].first, -mirrored_d.derivatives[j].first,
                  kDerivativePrecision);
      EXPECT_NEAR(d.derivatives[j].second, -mirrored_d.derivatives[j].second,
                  kSecondDerivativePrecision);
      EXPECT_NEAR(d.derivatives[j].third, -mirrored_d.derivatives[j].third,
                  kThirdDerivativePrecision);
    }
  }
}

// Ensure that the curves are scaled in x when node's x is scaled.
TEST_F(SplineTests, ScaleX) {
  static const float kScale = 100.0f;
  for (int i = 0; i < kNumSimpleSplines; ++i) {
    const CubicInit& init = kSimpleSplines[i];
    const CubicInit scaled_init = CubicInitScaleX(init, kScale);
    const float x_precision = init.width_x * kFixedPointEpsilon;
    const float y_precision =
        fabs(init.start_y - init.end_y) * kFixedPointEpsilon;

    GraphData d, scaled_d;
    GatherGraphData(init, &d);
    GatherGraphData(scaled_init, &scaled_d);

    EXPECT_EQ(d.points.size(), scaled_d.points.size());
    const int num_points = static_cast<int>(d.points.size());
    for (int j = 0; j < num_points; ++j) {
      EXPECT_NEAR(d.points[j].x, scaled_d.points[j].x / kScale, x_precision);
      EXPECT_NEAR(d.points[j].y, scaled_d.points[j].y, y_precision);
      EXPECT_NEAR(d.derivatives[j].first,
                  scaled_d.derivatives[j].first * kScale, kDerivativePrecision);
      EXPECT_NEAR(d.derivatives[j].second,
                  scaled_d.derivatives[j].second * kScale * kScale,
                  kSecondDerivativePrecision);
      EXPECT_NEAR(d.derivatives[j].third,
                  scaled_d.derivatives[j].third * kScale * kScale * kScale,
                  kThirdDerivativePrecision);
    }
  }
}

// YCalculatedSlowly should return the key-point Y values at key-point X values.
TEST_F(SplineTests, YSlowAtNodes) {
  for (CompactSplineIndex i = 0; i < short_spline_.num_nodes(); ++i) {
    EXPECT_NEAR(short_spline_.NodeY(i),
                short_spline_.YCalculatedSlowly(short_spline_.NodeX(i)),
                kNodeYPrecision);
  }
}

// BulkYs should return the proper start and end values.
TEST_F(SplineTests, BulkYsStartAndEnd) {
  static const int kMaxBulkYs = 5;

  // Get bulk data at several delta_xs, but always starting at the start of the
  // spline and ending at the end of the spline.
  // Then compare returned `ys` with start end end values of spline.
  for (size_t num_ys = 2; num_ys < kMaxBulkYs; ++num_ys) {
    float ys[kMaxBulkYs];
    float derivatives[kMaxBulkYs];
    CompactSpline::BulkYs(&short_spline_, 1, 0.0f,
                          short_spline_.EndX() / (num_ys - 1), num_ys, ys,
                          derivatives);

    EXPECT_NEAR(short_spline_.StartY(), ys[0], kNodeYPrecision);
    EXPECT_NEAR(short_spline_.EndY(), ys[num_ys - 1], kNodeYPrecision);
    EXPECT_NEAR(short_spline_.StartDerivative(), derivatives[0],
                kNodeYPrecision);
    EXPECT_NEAR(short_spline_.EndDerivative(), derivatives[num_ys - 1],
                kDerivativePrecision);
  }
}

// BulkYs should return the proper start and end values.
TEST_F(SplineTests, BulkYsVsSlowYs) {
  static const int kMaxBulkYs = 21;

  // Get bulk data at several delta_xs, but always starting at 3 delta_x
  // prior to start of the spline and ending at 3 delta_x after the end
  // of the spline.
  // Then compare returned `ys` with start end end values of spline.
  for (size_t num_ys = 2; num_ys < kMaxBulkYs - 6; ++num_ys) {
    // Collect `num_ys` evenly-spaced samples from short_spline_.
    float ys[kMaxBulkYs];
    float derivatives[kMaxBulkYs];
    const float delta_x = short_spline_.EndX() / (num_ys - 1);
    const float start_x = 0.0f - 3 * delta_x;
    const size_t num_points = num_ys + 6;
    CompactSpline::BulkYs(&short_spline_, 1, start_x, delta_x, num_points, ys,
                          derivatives);

    // Compare bulk samples to slowly calcuated samples.
    float x = start_x;
    for (size_t j = 0; j < num_points; ++j) {
      EXPECT_NEAR(short_spline_.YCalculatedSlowly(x), ys[j], kNodeYPrecision);
      EXPECT_NEAR(short_spline_.CalculatedSlowly(x, motive::kCurveDerivative),
                  derivatives[j], kDerivativePrecision);
      x += delta_x;
    }
  }
}

// BulkYs should return the proper start and end values.
TEST_F(SplineTests, BulkYsVec3) {
  static const int kDimensions = 3;
  static const int kNumYs = 16;

  // Make three copies of the spline data.
  CompactSpline splines[kDimensions];
  for (size_t d = 0; d < kDimensions; ++d) {
    splines[d] = short_spline_;
  }

  // Collect `num_ys` evenly-spaced samples from short_spline_.
  vec3_packed ys[kNumYs];
  memset(ys, 0xFF, sizeof(ys));
  const float delta_x = short_spline_.EndX() / (kNumYs - 1);
  CompactSpline::BulkYs<3>(splines, 0.0f, delta_x, kNumYs, ys);

  // Ensure all the values are being calculated.
  for (int j = 0; j < kNumYs; ++j) {
    const vec3 y(ys[j]);
    EXPECT_EQ(y.x, y.y);
    EXPECT_EQ(y.y, y.z);
  }
}

static const motive::UncompressedNode kUncompressed[] = {
    {0.0f, 0.0f, 0.0f},
    {1.0f, 0.5f, 0.03f},
    {1.5f, 0.6f, 0.02f},
    {3.0f, 0.0f, -0.04f},
};

static void CheckUncompressedNodes(const CompactSpline& spline,
                                   const motive::UncompressedNode* nodes,
                                   size_t num_nodes) {
  for (size_t i = 0; i < num_nodes; ++i) {
    const motive::UncompressedNode& n = nodes[i];
    EXPECT_NEAR(n.x, spline.NodeX(static_cast<CompactSplineIndex>(i)),
                kNodeXPrecision);
    EXPECT_NEAR(n.y, spline.NodeY(static_cast<CompactSplineIndex>(i)),
                kNodeYPrecision);
    EXPECT_NEAR(n.derivative,
                spline.NodeDerivative(static_cast<CompactSplineIndex>(i)),
                kDerivativePrecision);
  }
}

// Uncompressed nodes should be evaluated pretty much unchanged.
TEST_F(SplineTests, InitFromUncompressedNodes) {
  CompactSpline* spline = CompactSpline::CreateFromNodes(
      kUncompressed, MOTIVE_ARRAY_SIZE(kUncompressed));
  CheckUncompressedNodes(*spline, kUncompressed,
                         MOTIVE_ARRAY_SIZE(kUncompressed));
  CompactSpline::Destroy(spline);
}

// In-place construction from uncompressed nodes should be evaluated pretty
// much unchanged.
TEST_F(SplineTests, InitFromUncompressedNodesInPlace) {
  uint8_t spline_buf[1024];
  assert(sizeof(spline_buf) >=
         CompactSpline::Size(MOTIVE_ARRAY_SIZE(kUncompressed)));
  CompactSpline* spline = CompactSpline::CreateFromNodesInPlace(
      kUncompressed, MOTIVE_ARRAY_SIZE(kUncompressed), spline_buf);
  CheckUncompressedNodes(*spline, kUncompressed,
                         MOTIVE_ARRAY_SIZE(kUncompressed));
}

static const motive::UncompressedNode kUniformSpline[] = {
    {0.0f, 0.0f, 0.0f},   {1.0f, 0.5f, 0.03f},   {2.0f, 0.6f, 0.02f},
    {3.0f, 0.0f, -0.04f}, {4.0f, 0.03f, -0.02f}, {5.0f, 0.9f, -0.1f},
};

// CreateFromSpline of an already uniform spline should evaluate to the same
// spline.
TEST_F(SplineTests, InitFromSpline) {
  CompactSpline* uniform_spline = CompactSpline::CreateFromNodes(
      kUniformSpline, MOTIVE_ARRAY_SIZE(kUniformSpline));
  CompactSpline* spline = CompactSpline::CreateFromSpline(
      *uniform_spline, MOTIVE_ARRAY_SIZE(kUniformSpline));
  CheckUncompressedNodes(*spline, kUniformSpline,
                         MOTIVE_ARRAY_SIZE(kUniformSpline));
  CompactSpline::Destroy(spline);
  CompactSpline::Destroy(uniform_spline);
}

// CreateFromSpline of an already uniform spline should evaluate to the same
// spline. Test in-place construction.
TEST_F(SplineTests, InitFromSplineInPlace) {
  uint8_t uniform_spline_buf[1024];
  uint8_t spline_buf[1024];
  assert(sizeof(spline_buf) >=
             CompactSpline::Size(MOTIVE_ARRAY_SIZE(kUniformSpline)) &&
         sizeof(uniform_spline_buf) >=
             CompactSpline::Size(MOTIVE_ARRAY_SIZE(kUniformSpline)));
  CompactSpline* uniform_spline = CompactSpline::CreateFromNodesInPlace(
      kUniformSpline, MOTIVE_ARRAY_SIZE(kUniformSpline), uniform_spline_buf);
  CompactSpline* spline = CompactSpline::CreateFromSplineInPlace(
      *uniform_spline, MOTIVE_ARRAY_SIZE(kUniformSpline), spline_buf);
  CheckUncompressedNodes(*spline, kUniformSpline,
                         MOTIVE_ARRAY_SIZE(kUniformSpline));
}

TEST_F(SplineTests, YScaleAndOffset) {
  static const float kOffsets[] = {0.0f, 2.0f, 0.111f, 10.0f, -1.5f, -1.0f};
  static const float kScales[] = {1.0f, 2.0f, 0.1f, 1.1f, 0.0f, -1.0f, -1.3f};

  motive::SplinePlayback playback;
  for (size_t k = 0; k < MOTIVE_ARRAY_SIZE(kScales); ++k) {
    playback.y_offset = kOffsets[k];

    for (size_t j = 0; j < MOTIVE_ARRAY_SIZE(kScales); ++j) {
      playback.y_scale = kScales[j];

      for (int i = 0; i < kNumSimpleSplines; ++i) {
        const CubicInit& init = kSimpleSplines[i];

        GraphData d;
        GatherGraphData(init, &d, false, playback);
      }
    }
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
