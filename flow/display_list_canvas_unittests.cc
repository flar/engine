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

  bool renderAll(CvRenderer& cv_renderer, const DisplayList* display_list) {
    if (!renderWithColor(cv_renderer, display_list, SK_ColorBLUE)) {
      return false;
    }
    if (!renderWithColor(cv_renderer, display_list, SK_ColorGREEN)) {
      return false;
    }

    if (!renderWithBlend(cv_renderer, display_list, SkBlendMode::kSrcIn)) {
      return false;
    }
    if (!renderWithBlend(cv_renderer, display_list, SkBlendMode::kDstIn)) {
      return false;
    }

    return true;
  }

  bool renderWithColor(CvRenderer& cv_renderer, const DisplayList* display_list, SkColor color) {
    SkPaint paint;
    paint.setColor(color);

    DisplayListBuilder builder;
    builder.setColor(color);
    display_list->dispatch(builder);
    sk_sp<DisplayList> colored_list = builder.build();

    return render(cv_renderer, colored_list.get(), paint);
  }

  bool renderWithBlend(CvRenderer& cv_renderer, const DisplayList* display_list, SkBlendMode blend) {
    // half opaque cyan
    SkColor blendableColor = SkColorSetARGB(0x7f, 0x00, 0xff, 0xff);
    SkColor bg = SK_ColorWHITE;

    SkPaint paint;
    paint.setBlendMode(blend);
    paint.setColor(blendableColor);

    DisplayListBuilder builder;
    builder.setBlendMode(blend);
    builder.setColor(blendableColor);
    display_list->dispatch(builder);
    sk_sp<DisplayList> blended_list = builder.build();

    return render(cv_renderer, blended_list.get(), paint, &bg);
  }

  bool render(CvRenderer& cv_renderer, const DisplayList* display_list, SkPaint paint, SkColor* bg = nullptr) {
    sk_sp<SkSurface> surface1 = makeSurface(bg);
    cv_renderer(surface1->getCanvas(), paint);

    sk_sp<SkSurface> surface2 = makeSurface(bg);
    DisplayListCanvasRecorder dl_recorder(TestBounds);
    cv_renderer(&dl_recorder, paint);
    dl_recorder.builder()->build()->renderTo(surface2->getCanvas());
    if (!compare(surface1.get(), surface2.get(), bg)) {
      return false;
    }

    sk_sp<SkSurface> surface3 = makeSurface(bg);
    display_list->renderTo(surface3->getCanvas());
    if (!compare(surface1.get(), surface3.get(), bg)) {
      return false;
    }

    return true;
  }

  bool compare(SkSurface* surfaceA, SkSurface* surfaceB, SkColor* bg) {
    SkPixmap pixelsA;
    SkPixmap pixelsB;
    if (!surfaceA->peekPixels(&pixelsA) || !surfaceB->peekPixels(&pixelsB)) {
      return false;
    }
    if (pixelsA.width() != TestWidth || pixelsA.height() != TestHeight ||
        pixelsB.width() != TestWidth || pixelsB.height() != TestHeight) {
      return false;
    }
    bool pixel_touched = false;
    SkColor untouched = bg ? *bg : SK_ColorTRANSPARENT;
    for (int y = 0; y < TestHeight; y++) {
      for (int x = 0; x < TestWidth; x++) {
        SkColor pixel = pixelsA.getColor(x, y);
        if (!pixel_touched) {
          pixel_touched = (pixel != untouched);
        }
        if (pixelsA.getColor(x, y) != pixelsB.getColor(x, y)) {
          return false;
        }
      }
    }
    return pixel_touched;
  }

  sk_sp<SkSurface> makeSurface(SkColor *bg) {
    sk_sp<SkSurface> surface = SkSurface::MakeRasterN32Premul(TestWidth,
                                                              TestHeight);
    if (bg) {
      surface->getCanvas()->drawColor(*bg);
    }
    return surface;
  }

};

TEST(DisplayListCanvas, DrawPaint) {
  CanvasCompareTester tester;

  DisplayListBuilder builder;
  builder.drawPaint();
  sk_sp<DisplayList> dl = builder.build();
  bool success = tester.renderAll(
    [=](SkCanvas *canvas, SkPaint& paint) { canvas->drawPaint(paint); }, dl.get()
  );
  ASSERT_TRUE(success);
}

TEST(DisplayListCanvas, DrawColor) {
  CanvasCompareTester tester;

  DisplayListBuilder builder;
  builder.drawColor(SK_ColorMAGENTA, SkBlendMode::kSrcOver);
  sk_sp<DisplayList> dl = builder.build();
  bool success = tester.render(
    [=](SkCanvas *canvas, SkPaint& paint) { canvas->drawColor(SK_ColorMAGENTA); }, dl.get(), SkPaint()
  );
  ASSERT_TRUE(success);
}

TEST(DisplayListCanvas, DrawLine) {
  CanvasCompareTester tester;
  SkRect rect = RenderBounds.makeInset(20, 20);
  SkPoint p1 = SkPoint::Make(rect.fLeft, rect.fTop);
  SkPoint p2 = SkPoint::Make(rect.fRight, rect.fBottom);

  DisplayListBuilder builder;
  builder.drawLine(p1, p2);
  sk_sp<DisplayList> dl = builder.build();
  bool success = tester.renderAll(
    [=](SkCanvas *canvas, SkPaint& paint) { canvas->drawLine(p1, p2, paint); }, dl.get()
  );
  ASSERT_TRUE(success);
}

TEST(DisplayListCanvas, DrawRect) {
  CanvasCompareTester tester;

  DisplayListBuilder builder;
  builder.drawRect(RenderBounds);
  sk_sp<DisplayList> dl = builder.build();
  bool success = tester.renderAll(
    [=](SkCanvas *canvas, SkPaint& paint) { canvas->drawRect(RenderBounds, paint); }, dl.get()
  );
  ASSERT_TRUE(success);
}

TEST(DisplayListCanvas, DrawOval) {
  CanvasCompareTester tester;
  SkRect rect = RenderBounds.makeInset(0, 10);

  DisplayListBuilder builder;
  builder.drawOval(rect);
  sk_sp<DisplayList> dl = builder.build();
  bool success = tester.renderAll(
    [=](SkCanvas *canvas, SkPaint& paint) { canvas->drawOval(rect, paint); }, dl.get()
  );
  ASSERT_TRUE(success);
}

TEST(DisplayListCanvas, DrawCircle) {
  CanvasCompareTester tester;

  DisplayListBuilder builder;
  builder.drawCircle(TestCenter, TestRadius);
  sk_sp<DisplayList> dl = builder.build();
  bool success = tester.renderAll(
    [=](SkCanvas *canvas, SkPaint& paint) { canvas->drawCircle(TestCenter, TestRadius, paint); }, dl.get()
  );
  ASSERT_TRUE(success);
}

}  // namespace testing
}  // namespace flutter
