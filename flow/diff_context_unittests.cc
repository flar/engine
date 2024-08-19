// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/testing/diff_context_test.h"

namespace flutter {
namespace testing {

TEST_F(DiffContextTest, ClipAlignment) {
  MockLayerTree t1;
  t1.root()->Add(CreateDisplayListLayer(
      CreateDisplayList(SkRect::MakeLTRB(30, 30, 50, 50))));
  auto damage = DiffLayerTree(t1, MockLayerTree(), SkIRect::MakeEmpty(), 0, 0);
  EXPECT_EQ(damage.frame_damage, SkIRect::MakeLTRB(30, 30, 50, 50));
  EXPECT_EQ(damage.buffer_damage, SkIRect::MakeLTRB(30, 30, 50, 50));

  damage = DiffLayerTree(t1, MockLayerTree(), SkIRect::MakeEmpty(), 1, 1);
  EXPECT_EQ(damage.frame_damage, SkIRect::MakeLTRB(30, 30, 50, 50));
  EXPECT_EQ(damage.buffer_damage, SkIRect::MakeLTRB(30, 30, 50, 50));

  damage = DiffLayerTree(t1, MockLayerTree(), SkIRect::MakeEmpty(), 8, 1);
  EXPECT_EQ(damage.frame_damage, SkIRect::MakeLTRB(24, 30, 56, 50));
  EXPECT_EQ(damage.buffer_damage, SkIRect::MakeLTRB(24, 30, 56, 50));

  damage = DiffLayerTree(t1, MockLayerTree(), SkIRect::MakeEmpty(), 1, 8);
  EXPECT_EQ(damage.frame_damage, SkIRect::MakeLTRB(30, 24, 50, 56));
  EXPECT_EQ(damage.buffer_damage, SkIRect::MakeLTRB(30, 24, 50, 56));

  damage = DiffLayerTree(t1, MockLayerTree(), SkIRect::MakeEmpty(), 16, 16);
  EXPECT_EQ(damage.frame_damage, SkIRect::MakeLTRB(16, 16, 64, 64));
  EXPECT_EQ(damage.buffer_damage, SkIRect::MakeLTRB(16, 16, 64, 64));
}

TEST_F(DiffContextTest, DisjointDamage) {
  SkISize frame_size = SkISize::Make(90, 90);
  auto in_bounds_dl = CreateDisplayList(SkRect::MakeLTRB(30, 30, 50, 50));
  auto out_bounds_dl = CreateDisplayList(SkRect::MakeLTRB(100, 100, 120, 120));

  // We need both DisplayLists to be non-empty.
  ASSERT_FALSE(in_bounds_dl->bounds().isEmpty());
  ASSERT_FALSE(out_bounds_dl->bounds().isEmpty());

  // We need the in_bounds DisplayList to be inside the frame size.
  // We need the out_bounds DisplayList to be completely outside the frame.
  ASSERT_TRUE(SkRect::Make(frame_size).contains(in_bounds_dl->bounds()));
  ASSERT_FALSE(SkRect::Make(frame_size).intersects(out_bounds_dl->bounds()));

  MockLayerTree t1(frame_size);
  t1.root()->Add(CreateDisplayListLayer(in_bounds_dl));

  MockLayerTree t2(frame_size);
  // Include previous
  t2.root()->Add(CreateDisplayListLayer(in_bounds_dl));
  // Add a new layer that is out of frame bounds
  t2.root()->Add(CreateDisplayListLayer(out_bounds_dl));

  // Cannot use DiffLayerTree because it implicitly adds a clip layer
  // around the tree, but we want the out of bounds dl to not be pruned
  // to test the intersection code inside layer::Diff/ComputeDamage
  // damage = DiffLayerTree(t2, t1, SkIRect::MakeEmpty(), 0, 0);

  DiffContext dc(frame_size, t2.paint_region_map(), t1.paint_region_map(), true,
                 false);
  t2.root()->Diff(&dc, t1.root());
  auto damage = dc.ComputeDamage(SkIRect::MakeEmpty(), 0, 0);
  EXPECT_EQ(damage.frame_damage, SkIRect::MakeEmpty());
  EXPECT_EQ(damage.buffer_damage, SkIRect::MakeEmpty());
}

}  // namespace testing
}  // namespace flutter
