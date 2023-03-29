// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/display_list/dl_paint.h"

#include "flutter/display_list/utils/dl_comparable.h"
#include "gtest/gtest.h"

namespace flutter {
namespace testing {

TEST(DisplayListPaint, ConstructorDefaults) {
  DlPaint paint;
  EXPECT_FALSE(paint.isAntiAlias());
  EXPECT_FALSE(paint.isDither());
  EXPECT_FALSE(paint.isInvertColors());
  EXPECT_EQ(paint.getColor(), DlPaint::kDefaultColor);
  EXPECT_EQ(paint.getAlpha(), 0xFF);
  EXPECT_EQ(paint.getBlendMode(), DlBlendMode::kDefaultMode);
  EXPECT_EQ(paint.getDrawStyle(), DlDrawStyle::kDefaultStyle);
  EXPECT_EQ(paint.getStrokeCap(), DlStrokeCap::kDefaultCap);
  EXPECT_EQ(paint.getStrokeJoin(), DlStrokeJoin::kDefaultJoin);
  EXPECT_EQ(paint.getStrokeWidth(), DlPaint::kDefaultWidth);
  EXPECT_EQ(paint.getStrokeMiter(), DlPaint::kDefaultMiter);
  EXPECT_EQ(paint.getColorSource(), nullptr);
  EXPECT_EQ(paint.getColorFilter(), nullptr);
  EXPECT_EQ(paint.getImageFilter(), nullptr);
  EXPECT_EQ(paint.getMaskFilter(), nullptr);
  EXPECT_EQ(paint.getPathEffect(), nullptr);
  EXPECT_TRUE(paint.isDefault());
  EXPECT_EQ(paint, DlPaint::kDefault);

  EXPECT_EQ(DlBlendMode::kDefaultMode, DlBlendMode::kSrcOver);
  EXPECT_EQ(DlDrawStyle::kDefaultStyle, DlDrawStyle::kFill);
  EXPECT_EQ(DlStrokeCap::kDefaultCap, DlStrokeCap::kButt);
  EXPECT_EQ(DlStrokeJoin::kDefaultJoin, DlStrokeJoin::kMiter);

  EXPECT_EQ(DlPaint::kDefaultColor, DlColor::kBlack());
  EXPECT_EQ(DlPaint::kDefaultWidth, 0.0);
  EXPECT_EQ(DlPaint::kDefaultMiter, 4.0);

  EXPECT_EQ(paint, DlPaint());
  EXPECT_EQ(paint, DlPaint(DlColor::kBlack()));
  EXPECT_EQ(paint, DlPaint(0xFF000000));

  EXPECT_NE(paint, DlPaint().setAntiAlias(true));
  EXPECT_NE(paint, DlPaint().setDither(true));
  EXPECT_NE(paint, DlPaint().setInvertColors(true));
  EXPECT_NE(paint, DlPaint().setColor(DlColor::kGreen()));
  EXPECT_NE(paint, DlPaint(DlColor::kGreen()));
  EXPECT_NE(paint, DlPaint(0xFF00FF00));
  EXPECT_NE(paint, DlPaint().setAlpha(0x7f));
  EXPECT_NE(paint, DlPaint().setBlendMode(DlBlendMode::kDstIn));
  EXPECT_NE(paint, DlPaint().setDrawStyle(DlDrawStyle::kStrokeAndFill));
  EXPECT_NE(paint, DlPaint().setStrokeCap(DlStrokeCap::kRound));
  EXPECT_NE(paint, DlPaint().setStrokeJoin(DlStrokeJoin::kRound));
  EXPECT_NE(paint, DlPaint().setStrokeWidth(6));
  EXPECT_NE(paint, DlPaint().setStrokeMiter(7));

  auto color_source = DlColorSource::MakeColor(DlColor::kMagenta());
  EXPECT_NE(paint, DlPaint().setColorSource(color_source));

  auto color_filter =
      DlBlendColorFilter::Make(DlColor::kYellow(), DlBlendMode::kSrcIn);
  EXPECT_NE(paint, DlPaint().setColorFilter(color_filter));

  auto image_filter = DlBlurImageFilter::Make(1.3, 4.7, DlTileMode::kClamp);
  EXPECT_NE(paint, DlPaint().setImageFilter(image_filter));

  auto mask_filter = DlMaskFilter::MakeBlur(DlBlurStyle::kInner, 3.14);
  EXPECT_NE(paint, DlPaint().setMaskFilter(mask_filter));

  const SkScalar dashes[3] = {10, 5, 2};
  auto path_effect = DlPathEffect::MakeDash(dashes, 3, 2.0);
  EXPECT_NE(paint, DlPaint().setPathEffect(path_effect));
}

TEST(DisplayListPaint, NullPointerSetGet) {
  DlColorSource* null_color_source = nullptr;
  DlColorFilter* null_color_filter = nullptr;
  DlImageFilter* null_image_filter = nullptr;
  DlMaskFilter* null_mask_filter = nullptr;
  DlPathEffect* null_path_effect = nullptr;
  DlPaint paint;
  EXPECT_EQ(paint.setColorSource(null_color_source).getColorSource(), nullptr);
  EXPECT_EQ(paint.setColorFilter(null_color_filter).getColorFilter(), nullptr);
  EXPECT_EQ(paint.setImageFilter(null_image_filter).getImageFilter(), nullptr);
  EXPECT_EQ(paint.setMaskFilter(null_mask_filter).getMaskFilter(), nullptr);
  EXPECT_EQ(paint.setPathEffect(null_path_effect).getPathEffect(), nullptr);
}

TEST(DisplayListPaint, NullSharedPointerSetGet) {
  dl_shared<DlColorSource> null_color_source;
  dl_shared<DlColorFilter> null_color_filter;
  dl_shared<DlImageFilter> null_image_filter;
  dl_shared<DlMaskFilter> null_mask_filter;
  dl_shared<DlPathEffect> null_path_effect;
  DlPaint paint;
  EXPECT_EQ(paint.setColorSource(null_color_source).getColorSource(), nullptr);
  EXPECT_EQ(paint.setColorFilter(null_color_filter).getColorFilter(), nullptr);
  EXPECT_EQ(paint.setImageFilter(null_image_filter).getImageFilter(), nullptr);
  EXPECT_EQ(paint.setMaskFilter(null_mask_filter).getMaskFilter(), nullptr);
  EXPECT_EQ(paint.setPathEffect(null_path_effect).getPathEffect(), nullptr);
}

TEST(DisplayListPaint, ChainingConstructor) {
  const SkScalar dashes[3] = {10, 5, 2};
  DlPaint paint =
      DlPaint()                                                           //
          .setAntiAlias(true)                                             //
          .setDither(true)                                                //
          .setInvertColors(true)                                          //
          .setColor(DlColor::kGreen())                                    //
          .setAlpha(0x7F)                                                 //
          .setBlendMode(DlBlendMode::kLuminosity)                         //
          .setDrawStyle(DlDrawStyle::kStrokeAndFill)                      //
          .setStrokeCap(DlStrokeCap::kSquare)                             //
          .setStrokeJoin(DlStrokeJoin::kBevel)                            //
          .setStrokeWidth(42)                                             //
          .setStrokeMiter(1.5)                                            //
          .setColorSource(DlColorColorSource::Make(DlColor::kMagenta()))  //
          .setColorFilter(
              DlBlendColorFilter::Make(DlColor::kYellow(), DlBlendMode::kDstIn))
          .setImageFilter(DlBlurImageFilter::Make(1.3, 4.7, DlTileMode::kClamp))
          .setMaskFilter(DlMaskFilter::MakeBlur(DlBlurStyle::kInner, 3.14))
          .setPathEffect(DlPathEffect::MakeDash(dashes, 3, 2.0));
  EXPECT_TRUE(paint.isAntiAlias());
  EXPECT_TRUE(paint.isDither());
  EXPECT_TRUE(paint.isInvertColors());
  EXPECT_EQ(paint.getColor(), DlColor::kGreen().withAlpha(0x7F));
  EXPECT_EQ(paint.getAlpha(), 0x7F);
  EXPECT_EQ(paint.getBlendMode(), DlBlendMode::kLuminosity);
  EXPECT_EQ(paint.getDrawStyle(), DlDrawStyle::kStrokeAndFill);
  EXPECT_EQ(paint.getStrokeCap(), DlStrokeCap::kSquare);
  EXPECT_EQ(paint.getStrokeJoin(), DlStrokeJoin::kBevel);
  EXPECT_EQ(paint.getStrokeWidth(), 42);
  EXPECT_EQ(paint.getStrokeMiter(), 1.5);
  EXPECT_TRUE(Equals(paint.getColorSource(),
                     DlColorSource::MakeColor(DlColor::kMagenta())));
  EXPECT_TRUE(Equals(
      paint.getColorFilter(),
      DlColorFilter::MakeBlend(DlColor::kYellow(), DlBlendMode::kDstIn)));
  EXPECT_TRUE(Equals(paint.getImageFilter(),
                     DlBlurImageFilter::Make(1.3, 4.7, DlTileMode::kClamp)));
  EXPECT_TRUE(Equals(paint.getMaskFilter(),
                     DlMaskFilter::MakeBlur(DlBlurStyle::kInner, 3.14)));
  EXPECT_TRUE(
      Equals(paint.getPathEffect(), DlPathEffect::MakeDash(dashes, 3, 2.0)));

  EXPECT_NE(paint, DlPaint());
}

}  // namespace testing
}  // namespace flutter
