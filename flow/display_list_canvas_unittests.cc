// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/display_list_canvas.h"
// #include "flutter/lib/ui/painting/gradient.h"

#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/effects/SkGradientShader.h"
#include "third_party/skia/include/effects/SkImageFilters.h"

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
    renderWithAttributes(cv_renderer, dl_renderer);
    renderWithTransforms(cv_renderer, dl_renderer);
    renderWithClips(cv_renderer, dl_renderer);
  }

  static void renderWithAttributes(CvRenderer& cv_renderer,
                                   DlRenderer& dl_renderer) {
    renderWith([=](SkCanvas*, SkPaint& p) {},  //
               [=](DisplayListBuilder& d) {},  //
               cv_renderer, dl_renderer);

    renderWith([=](SkCanvas*, SkPaint& p) { p.setAntiAlias(true); },  //
               [=](DisplayListBuilder& b) { b.setAA(true); },         //
               cv_renderer, dl_renderer);
    renderWith([=](SkCanvas*, SkPaint& p) { p.setAntiAlias(false); },  //
               [=](DisplayListBuilder& b) { b.setAA(false); },         //
               cv_renderer, dl_renderer);

    // Not testing setInvertColors here because there is no SkPaint version

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

    renderWithStrokes(cv_renderer, dl_renderer);

    // Not testing FilterQuality here because there is no SkPaint version

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

    {
      sk_sp<SkImageFilter> filter =
          SkImageFilters::Blur(5.0, 5.0, SkTileMode::kDecal, nullptr, nullptr);
      {
        renderWith([=](SkCanvas*, SkPaint& p) { p.setImageFilter(filter); },
                   [=](DisplayListBuilder& b) { b.setImageFilter(filter); },
                   cv_renderer, dl_renderer);
      }
      ASSERT_TRUE(filter->unique());
    }

    {
      constexpr float rotate_color_matrix[20] = {
          0, 1, 0, 0, 0,  //
          0, 0, 1, 0, 0,  //
          1, 0, 0, 0, 0,  //
          0, 0, 0, 1, 0,  //
      };
      sk_sp<SkColorFilter> filter = SkColorFilters::Matrix(rotate_color_matrix);
      {
        SkColor bg = SK_ColorWHITE;
        renderWith(
            [=](SkCanvas*, SkPaint& p) {
              p.setColor(SK_ColorYELLOW);
              p.setColorFilter(filter);
            },
            [=](DisplayListBuilder& b) {
              b.setColor(SK_ColorYELLOW);
              b.setColorFilter(filter);
            },
            cv_renderer, dl_renderer, &bg);
      }
      ASSERT_TRUE(filter->unique());
    }

    {
      sk_sp<SkMaskFilter> filter =
          SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 5.0);
      {
        renderWith([=](SkCanvas*, SkPaint& p) { p.setMaskFilter(filter); },
                   [=](DisplayListBuilder& b) { b.setMaskFilter(filter); },
                   cv_renderer, dl_renderer);
      }
      ASSERT_TRUE(filter->unique());
      {
        renderWith([=](SkCanvas*, SkPaint& p) { p.setMaskFilter(filter); },
                   [=](DisplayListBuilder& b) {
                     b.setMaskBlurFilter(kNormal_SkBlurStyle, 5.0);
                   },
                   cv_renderer, dl_renderer);
      }
      ASSERT_TRUE(filter->unique());
    }

    {
      SkPoint end_points[] = {
          SkPoint::Make(RenderBounds.fLeft, RenderBounds.fTop),
          SkPoint::Make(RenderBounds.fRight, RenderBounds.fBottom),
      };
      SkColor colors[] = {
          SK_ColorGREEN,
          SK_ColorYELLOW,
          SK_ColorBLUE,
      };
      float stops[] = {
          0.0,
          0.5,
          1.0,
      };
      sk_sp<SkShader> shader = SkGradientShader::MakeLinear(
          end_points, colors, stops, 3, SkTileMode::kMirror, 0, nullptr);
      {
        renderWith([=](SkCanvas*, SkPaint& p) { p.setShader(shader); },
                   [=](DisplayListBuilder& b) { b.setShader(shader); },
                   cv_renderer, dl_renderer);
      }
      ASSERT_TRUE(shader->unique());
    }
  }

  static void renderWithStrokes(CvRenderer& cv_renderer,
                                DlRenderer& dl_renderer) {
    renderWith(
        [=](SkCanvas*, SkPaint& p) { p.setStyle(SkPaint::kFill_Style); },
        [=](DisplayListBuilder& b) { b.setDrawStyle(SkPaint::kFill_Style); },
        cv_renderer, dl_renderer);
    renderWith(
        [=](SkCanvas*, SkPaint& p) { p.setStyle(SkPaint::kStroke_Style); },
        [=](DisplayListBuilder& b) { b.setDrawStyle(SkPaint::kStroke_Style); },
        cv_renderer, dl_renderer);

    renderWith(
        [=](SkCanvas*, SkPaint& p) {
          p.setStyle(SkPaint::kFill_Style);
          p.setStrokeWidth(10.0);
        },
        [=](DisplayListBuilder& b) {
          b.setDrawStyle(SkPaint::kFill_Style);
          b.setStrokeWidth(10.0);
        },
        cv_renderer, dl_renderer);

    renderWith(
        [=](SkCanvas*, SkPaint& p) {
          p.setStyle(SkPaint::kStroke_Style);
          p.setStrokeWidth(10.0);
        },
        [=](DisplayListBuilder& b) {
          b.setDrawStyle(SkPaint::kStroke_Style);
          b.setStrokeWidth(10.0);
        },
        cv_renderer, dl_renderer);
    renderWith(
        [=](SkCanvas*, SkPaint& p) {
          p.setStyle(SkPaint::kStroke_Style);
          p.setStrokeWidth(5.0);
        },
        [=](DisplayListBuilder& b) {
          b.setDrawStyle(SkPaint::kStroke_Style);
          b.setStrokeWidth(5.0);
        },
        cv_renderer, dl_renderer);

    renderWith(
        [=](SkCanvas*, SkPaint& p) {
          p.setStyle(SkPaint::kStroke_Style);
          p.setStrokeWidth(5.0);
          p.setStrokeCap(SkPaint::kButt_Cap);
        },
        [=](DisplayListBuilder& b) {
          b.setDrawStyle(SkPaint::kStroke_Style);
          b.setStrokeWidth(5.0);
          b.setCap(SkPaint::kButt_Cap);
        },
        cv_renderer, dl_renderer);
    renderWith(
        [=](SkCanvas*, SkPaint& p) {
          p.setStyle(SkPaint::kStroke_Style);
          p.setStrokeWidth(5.0);
          p.setStrokeCap(SkPaint::kRound_Cap);
        },
        [=](DisplayListBuilder& b) {
          b.setDrawStyle(SkPaint::kStroke_Style);
          b.setStrokeWidth(5.0);
          b.setCap(SkPaint::kRound_Cap);
        },
        cv_renderer, dl_renderer);

    renderWith(
        [=](SkCanvas*, SkPaint& p) {
          p.setStyle(SkPaint::kStroke_Style);
          p.setStrokeWidth(5.0);
          p.setStrokeJoin(SkPaint::kBevel_Join);
        },
        [=](DisplayListBuilder& b) {
          b.setDrawStyle(SkPaint::kStroke_Style);
          b.setStrokeWidth(5.0);
          b.setJoin(SkPaint::kBevel_Join);
        },
        cv_renderer, dl_renderer);
    renderWith(
        [=](SkCanvas*, SkPaint& p) {
          p.setStyle(SkPaint::kStroke_Style);
          p.setStrokeWidth(5.0);
          p.setStrokeJoin(SkPaint::kRound_Join);
        },
        [=](DisplayListBuilder& b) {
          b.setDrawStyle(SkPaint::kStroke_Style);
          b.setStrokeWidth(5.0);
          b.setJoin(SkPaint::kRound_Join);
        },
        cv_renderer, dl_renderer);

    renderWith(
        [=](SkCanvas*, SkPaint& p) {
          p.setStyle(SkPaint::kStroke_Style);
          p.setStrokeWidth(5.0);
          p.setStrokeMiter(100.0);
          p.setStrokeJoin(SkPaint::kMiter_Join);
        },
        [=](DisplayListBuilder& b) {
          b.setDrawStyle(SkPaint::kStroke_Style);
          b.setStrokeWidth(5.0);
          b.setMiterLimit(100.0);
          b.setJoin(SkPaint::kMiter_Join);
        },
        cv_renderer, dl_renderer);

    renderWith(
        [=](SkCanvas*, SkPaint& p) {
          p.setStyle(SkPaint::kStroke_Style);
          p.setStrokeWidth(5.0);
          p.setStrokeMiter(0.0);
          p.setStrokeJoin(SkPaint::kMiter_Join);
        },
        [=](DisplayListBuilder& b) {
          b.setDrawStyle(SkPaint::kStroke_Style);
          b.setStrokeWidth(5.0);
          b.setMiterLimit(0.0);
          b.setJoin(SkPaint::kMiter_Join);
        },
        cv_renderer, dl_renderer);
  }

  static void renderWithTransforms(CvRenderer& cv_renderer,
                                   DlRenderer& dl_renderer) {
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
  }

  static void renderWithClips(CvRenderer& cv_renderer,
                              DlRenderer& dl_renderer) {
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
    sk_sp<SkSurface> ref_surface = makeSurface(bg);
    SkPaint paint1;
    cv_setup(ref_surface->getCanvas(), paint1);
    cv_render(ref_surface->getCanvas(), paint1);
    SkPixmap ref_pixels;
    ASSERT_TRUE(ref_surface->peekPixels(&ref_pixels));
    ASSERT_EQ(ref_pixels.width(), TestWidth);
    ASSERT_EQ(ref_pixels.height(), TestHeight);
    SkColor untouched = bg ? *bg : SK_ColorTRANSPARENT;
    int pixels_touched = 0;
    for (int y = 0; y < TestHeight; y++) {
      for (int x = 0; x < TestWidth; x++) {
        if (ref_pixels.getColor(x, y) != untouched) {
          pixels_touched++;
        }
      }
    }
    ASSERT_GT(pixels_touched, 0);

    {
      // This sequence plays the provided equivalently constructed
      // DisplayList onto the SkCanvas of the surface
      // DisplayList => direct rendering
      sk_sp<SkSurface> test_surface = makeSurface(bg);
      DisplayListBuilder builder;
      dl_setup(builder);
      dl_render(builder);
      builder.build()->renderTo(test_surface->getCanvas());
      compareToReference(test_surface.get(), &ref_pixels);
    }

    {
      // This sequence renders SkCanvas calls to a DisplayList and then
      // plays them back on SkCanvas to SkSurface
      // SkSurface calls => DisplayList => rendering
      sk_sp<SkSurface> test_surface = makeSurface(bg);
      DisplayListCanvasRecorder dl_recorder(TestBounds);
      SkPaint test_paint;
      cv_setup(&dl_recorder, test_paint);
      cv_render(&dl_recorder, test_paint);
      dl_recorder.builder()->build()->renderTo(test_surface->getCanvas());
      compareToReference(test_surface.get(), &ref_pixels);
    }
  }

  static void compareToReference(SkSurface* test_surface, SkPixmap* reference) {
    SkPixmap test_pixels;
    ASSERT_TRUE(test_surface->peekPixels(&test_pixels));
    ASSERT_EQ(test_pixels.width(), TestWidth);
    ASSERT_EQ(test_pixels.height(), TestHeight);

    int pixels_different = 0;
    for (int y = 0; y < TestHeight; y++) {
      for (int x = 0; x < TestWidth; x++) {
        if (test_pixels.getColor(x, y) != reference->getColor(x, y)) {
          pixels_different++;
        }
      }
    }
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
