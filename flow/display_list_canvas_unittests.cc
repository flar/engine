// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/display_list_canvas.h"

#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkSurface.h"

#include <cmath>

#include "gtest/gtest.h"

namespace flutter {
namespace testing {

constexpr int TestWidth = 200;
constexpr int TestHeight = 200;
constexpr int TestRadius = 50;
constexpr SkPoint TestCenter = SkPoint::Make(TestWidth / 2, TestHeight / 2);
constexpr SkRect TestBounds = SkRect::MakeWH(TestWidth, TestHeight);
constexpr SkRect RenderBounds = SkRect::MakeXYWH(TestWidth / 4,
                                                 TestWidth / 4,
                                                 TestWidth / 2,
                                                 TestHeight / 2);

class CanvasCompareTester {
 public:
  typedef const std::function<void(SkCanvas*, SkPaint&)> CvRenderer;
  typedef const std::function<void(DisplayListBuilder&)> DlRenderer;

  static void renderAll(CvRenderer& cv_renderer, DlRenderer& dl_renderer) {
    renderWith([=](SkCanvas*, SkPaint& p) {},  //
               [=](DisplayListBuilder& d) {},  //
               cv_renderer, dl_renderer);

    renderWith([=](SkCanvas*, SkPaint& p) { p.setAntiAlias(true); },  //
               [=](DisplayListBuilder& b) { b.setAA(true); },         //
               cv_renderer, dl_renderer);
    renderWith([=](SkCanvas*, SkPaint& p) { p.setAntiAlias(false); },  //
               [=](DisplayListBuilder& b) { b.setAA(false); },         //
               cv_renderer, dl_renderer);

    renderWith([=](SkCanvas*, SkPaint& p) { p.setDither(true); },  //
               [=](DisplayListBuilder& b) { b.setDither(true); },  //
               cv_renderer, dl_renderer);
    renderWith([=](SkCanvas*, SkPaint& p) { p.setDither(false); },  //
               [=](DisplayListBuilder& b) { b.setDither(false); },  //
               cv_renderer, dl_renderer);

    renderWith([=](SkCanvas*, SkPaint& p) { p.setColor(SK_ColorBLUE); },  //
               [=](DisplayListBuilder& b) { b.setColor(SK_ColorBLUE); },  //
               cv_renderer, dl_renderer);
    renderWith([=](SkCanvas*, SkPaint& p) { p.setColor(SK_ColorGREEN); },  //
               [=](DisplayListBuilder& b) { b.setColor(SK_ColorGREEN); },  //
               cv_renderer, dl_renderer);

    {
      // half opaque cyan
      SkColor blendableColor = SkColorSetARGB(0x7f, 0x00, 0xff, 0xff);
      SkColor bg = SK_ColorWHITE;

      renderWith(
          [=](SkCanvas*, SkPaint& p) {
            p.setBlendMode(SkBlendMode::kSrcIn);
            p.setColor(blendableColor);
          },
          [=](DisplayListBuilder& b) {
            b.setBlendMode(SkBlendMode::kSrcIn);
            b.setColor(blendableColor);
          },
          cv_renderer, dl_renderer, &bg);
      renderWith(
          [=](SkCanvas*, SkPaint& p) {
            p.setBlendMode(SkBlendMode::kDstIn);
            p.setColor(blendableColor);
          },
          [=](DisplayListBuilder& b) {
            b.setBlendMode(SkBlendMode::kDstIn);
            b.setColor(blendableColor);
          },
          cv_renderer, dl_renderer, &bg);
    }

    renderWith([=](SkCanvas* c, SkPaint&) { c->translate(5, 10); },  //
               [=](DisplayListBuilder& b) { b.translate(5, 10); },   //
               cv_renderer, dl_renderer);
    renderWith([=](SkCanvas* c, SkPaint&) { c->scale(0.95, 0.95); },  //
               [=](DisplayListBuilder& b) { b.scale(0.95, 0.95); },   //
               cv_renderer, dl_renderer);
    renderWith([=](SkCanvas* c, SkPaint&) { c->rotate(5); },  //
               [=](DisplayListBuilder& b) { b.rotate(5); },   //
               cv_renderer, dl_renderer);
    renderWith([=](SkCanvas* c, SkPaint&) { c->skew(0.05, 0.05); },  //
               [=](DisplayListBuilder& b) { b.skew(0.05, 0.05); },   //
               cv_renderer, dl_renderer);

    renderWith(
        [=](SkCanvas* c, SkPaint&) {
          c->clipRect(RenderBounds.makeInset(25.5, 25.5),  //
                      SkClipOp::kIntersect, false);
        },
        [=](DisplayListBuilder& b) {
          b.clipRect(RenderBounds.makeInset(25.5, 25.5),  //
                     false, SkClipOp::kIntersect);
        },
        cv_renderer, dl_renderer);
    renderWith(
        [=](SkCanvas* c, SkPaint&) {
          c->clipRect(RenderBounds.makeInset(25.5, 25.5),  //
                      SkClipOp::kIntersect, true);
        },
        [=](DisplayListBuilder& b) {
          b.clipRect(RenderBounds.makeInset(25.5, 25.5),  //
                     true, SkClipOp::kIntersect);
        },
        cv_renderer, dl_renderer);
    renderWith(
        [=](SkCanvas* c, SkPaint&) {
          c->clipRect(RenderBounds.makeInset(25.5, 25.5),  //
                      SkClipOp::kDifference, false);
        },
        [=](DisplayListBuilder& b) {
          b.clipRect(RenderBounds.makeInset(25.5, 25.5),  //
                     false, SkClipOp::kDifference);
        },
        cv_renderer, dl_renderer);
  }

  static void renderWith(CvRenderer& cv_setup,
                         DlRenderer& dl_setup,
                         CvRenderer& cv_render,
                         DlRenderer& dl_render,
                         const SkColor* bg = nullptr) {
    // surface1 is direct rendering via SkCanvas to SkSurface
    sk_sp<SkSurface> surface1 = makeSurface(bg);
    SkPaint paint1;
    cv_setup(surface1->getCanvas(), paint1);
    cv_render(surface1->getCanvas(), paint1);

    // surface2 renders SkCanvas calls to DisplayList and then
    // plays them back on SkCanvas to SkSurface
    sk_sp<SkSurface> surface2 = makeSurface(bg);
    DisplayListCanvasRecorder dl_recorder(TestBounds);
    SkPaint paint2;
    cv_setup(&dl_recorder, paint2);
    cv_render(&dl_recorder, paint2);
    dl_recorder.builder()->build()->renderTo(surface2->getCanvas());
    compare(surface1.get(), surface2.get(), bg);

    // surface3 plays the provided equivalently constructed
    // DisplayList onto the SkCanvas of the surface
    sk_sp<SkSurface> surface3 = makeSurface(bg);
    DisplayListBuilder builder;
    dl_setup(builder);
    dl_render(builder);
    builder.build()->renderTo(surface3->getCanvas());
    compare(surface1.get(), surface3.get(), bg);
  }

  static void compare(SkSurface* surfaceA,
                      SkSurface* surfaceB,
                      const SkColor* bg) {
    SkPixmap pixelsA, pixelsB;
    ASSERT_TRUE(surfaceA->peekPixels(&pixelsA));
    ASSERT_EQ(pixelsA.width(), TestWidth);
    ASSERT_EQ(pixelsA.height(), TestHeight);
    ASSERT_TRUE(surfaceB->peekPixels(&pixelsB));
    ASSERT_EQ(pixelsB.width(), TestWidth);
    ASSERT_EQ(pixelsB.height(), TestHeight);

    int pixels_touched = 0;
    int pixels_different = 0;
    SkColor untouched = bg ? *bg : SK_ColorTRANSPARENT;
    for (int y = 0; y < TestHeight; y++) {
      for (int x = 0; x < TestWidth; x++) {
        if (pixelsA.getColor(x, y) != untouched) {
          pixels_touched++;
        }
        if (pixelsA.getColor(x, y) != pixelsB.getColor(x, y)) {
          pixels_different++;
        }
      }
    }
    ASSERT_GT(pixels_touched, 0);
    ASSERT_EQ(pixels_different, 0);
  }

  static sk_sp<SkSurface> makeSurface(const SkColor* bg) {
    sk_sp<SkSurface> surface =
        SkSurface::MakeRasterN32Premul(TestWidth, TestHeight);
    if (bg) {
      surface->getCanvas()->drawColor(*bg);
    }
    return surface;
  }
};

TEST(DisplayListCanvas, DrawPaint) {
  CanvasCompareTester::renderAll(
      [=](SkCanvas* canvas, SkPaint& paint) {  //
        canvas->drawPaint(paint);
      },
      [=](DisplayListBuilder& builder) {  //
        builder.drawPaint();
      });
}

TEST(DisplayListCanvas, DrawColor) {
  CanvasCompareTester::renderWith(             //
      [=](SkCanvas*, SkPaint& p) {},           //
      [=](DisplayListBuilder& b) {},           //
      [=](SkCanvas* canvas, SkPaint& paint) {  //
        canvas->drawColor(SK_ColorMAGENTA);
      },
      [=](DisplayListBuilder& builder) {  //
        builder.drawColor(SK_ColorMAGENTA, SkBlendMode::kSrcOver);
      });
}

TEST(DisplayListCanvas, DrawLine) {
  SkRect rect = RenderBounds.makeInset(20, 20);
  SkPoint p1 = SkPoint::Make(rect.fLeft, rect.fTop);
  SkPoint p2 = SkPoint::Make(rect.fRight, rect.fBottom);

  CanvasCompareTester::renderAll(
      [=](SkCanvas* canvas, SkPaint& paint) {  //
        canvas->drawLine(p1, p2, paint);
      },
      [=](DisplayListBuilder& builder) {  //
        builder.drawLine(p1, p2);
      });
}

TEST(DisplayListCanvas, DrawRect) {
  CanvasCompareTester::renderAll(
      [=](SkCanvas* canvas, SkPaint& paint) {  //
        canvas->drawRect(RenderBounds, paint);
      },
      [=](DisplayListBuilder& builder) {  //
        builder.drawRect(RenderBounds);
      });
}

TEST(DisplayListCanvas, DrawOval) {
  SkRect rect = RenderBounds.makeInset(0, 10);

  CanvasCompareTester::renderAll(
      [=](SkCanvas* canvas, SkPaint& paint) {  //
        canvas->drawOval(rect, paint);
      },
      [=](DisplayListBuilder& builder) {  //
        builder.drawOval(rect);
      });
}

TEST(DisplayListCanvas, DrawCircle) {
  CanvasCompareTester::renderAll(
      [=](SkCanvas* canvas, SkPaint& paint) {  //
        canvas->drawCircle(TestCenter, TestRadius, paint);
      },
      [=](DisplayListBuilder& builder) {  //
        builder.drawCircle(TestCenter, TestRadius);
      });
}

}  // namespace testing
}  // namespace flutter
