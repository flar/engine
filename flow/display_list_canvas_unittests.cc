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
  typedef const std::function<void(SkPaint&)> CvSetup;
  typedef const std::function<void(SkCanvas*, SkPaint&)> CvRenderer;
  typedef const std::function<void(DisplayListBuilder&)> DlRenderer;

  static void renderAll(CvRenderer& cv_renderer, DlRenderer& dl_renderer) {
    renderWith([=](SkPaint& p) {},             //
               [=](DisplayListBuilder& d) {},  //
               cv_renderer, dl_renderer);

    renderWith([=](SkPaint& p) { p.setAntiAlias(true); },      //
               [=](DisplayListBuilder& b) { b.setAA(true); },  //
               cv_renderer, dl_renderer);
    renderWith([=](SkPaint& p) { p.setAntiAlias(false); },      //
               [=](DisplayListBuilder& b) { b.setAA(false); },  //
               cv_renderer, dl_renderer);

    renderWith([=](SkPaint& p) { p.setDither(true); },             //
               [=](DisplayListBuilder& b) { b.setDither(true); },  //
               cv_renderer, dl_renderer);
    renderWith([=](SkPaint& p) { p.setDither(false); },             //
               [=](DisplayListBuilder& b) { b.setDither(false); },  //
               cv_renderer, dl_renderer);

    renderWith([=](SkPaint& p) { p.setColor(SK_ColorBLUE); },             //
               [=](DisplayListBuilder& b) { b.setColor(SK_ColorBLUE); },  //
               cv_renderer, dl_renderer);
    renderWith([=](SkPaint& p) { p.setColor(SK_ColorGREEN); },             //
               [=](DisplayListBuilder& b) { b.setColor(SK_ColorGREEN); },  //
               cv_renderer, dl_renderer);

    {
      // half opaque cyan
      SkColor blendableColor = SkColorSetARGB(0x7f, 0x00, 0xff, 0xff);
      SkColor bg = SK_ColorWHITE;

      renderWith(
          [=](SkPaint& p) {
            p.setBlendMode(SkBlendMode::kSrcIn);
            p.setColor(blendableColor);
          },
          [=](DisplayListBuilder& b) {
            b.setBlendMode(SkBlendMode::kSrcIn);
            b.setColor(blendableColor);
          },
          cv_renderer, dl_renderer, &bg);
      renderWith(
          [=](SkPaint& p) {
            p.setBlendMode(SkBlendMode::kDstIn);
            p.setColor(blendableColor);
          },
          [=](DisplayListBuilder& b) {
            b.setBlendMode(SkBlendMode::kDstIn);
            b.setColor(blendableColor);
          },
          cv_renderer, dl_renderer, &bg);
    }
  }

  static void renderWith(CvSetup& cv_setup,
                         DlRenderer& dl_setup,
                         CvRenderer& cv_render,
                         DlRenderer& dl_render,
                         const SkColor* bg = nullptr) {
    SkPaint paint;
    cv_setup(paint);

    DisplayListBuilder builder;
    dl_setup(builder);
    dl_render(builder);
    sk_sp<DisplayList> display_list = builder.build();

    render(cv_render, display_list.get(), paint, bg);
  }

  static void render(CvRenderer& cv_renderer,
                     const DisplayList* display_list,
                     SkPaint paint,
                     const SkColor* bg = nullptr) {
    sk_sp<SkSurface> surface1 = makeSurface(bg);
    cv_renderer(surface1->getCanvas(), paint);

    sk_sp<SkSurface> surface2 = makeSurface(bg);
    DisplayListCanvasRecorder dl_recorder(TestBounds);
    cv_renderer(&dl_recorder, paint);
    dl_recorder.builder()->build()->renderTo(surface2->getCanvas());
    compare(surface1.get(), surface2.get(), bg);

    sk_sp<SkSurface> surface3 = makeSurface(bg);
    display_list->renderTo(surface3->getCanvas());
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
      [=](SkPaint& p) {},                      //
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
