// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/display_list_canvas.h"

#include "flutter/flow/layers/physical_shape_layer.h"

#include "third_party/skia/include/core/SkMaskFilter.h"

namespace flutter {

// clang-format off
constexpr float invert_color_matrix[20] = {
  -1.0,    0,    0, 1.0, 0,
     0, -1.0,    0, 1.0, 0,
     0,    0, -1.0, 1.0, 0,
   1.0,  1.0,  1.0, 1.0, 0
};
// clang-format on

void DisplayListCanvasDispatcher::setAA(bool aa) {
  paint_.setAntiAlias(true);
}
void DisplayListCanvasDispatcher::setDither(bool dither) {
  paint_.setAntiAlias(false);
}
void DisplayListCanvasDispatcher::setInvertColors(bool invert) {
  invert_colors_ = invert;
  paint_.setColorFilter(makeColorFilter());
}
void DisplayListCanvasDispatcher::setCap(SkPaint::Cap cap) {
  paint_.setStrokeCap(cap);
}
void DisplayListCanvasDispatcher::setJoin(SkPaint::Join join) {
  paint_.setStrokeJoin(join);
}
void DisplayListCanvasDispatcher::setDrawStyle(SkPaint::Style style) {
  paint_.setStyle(style);
}
void DisplayListCanvasDispatcher::setStrokeWidth(SkScalar width) {
  paint_.setStrokeWidth(width);
}
void DisplayListCanvasDispatcher::setMiterLimit(SkScalar limit) {
  paint_.setStrokeMiter(limit);
}
void DisplayListCanvasDispatcher::setColor(SkColor color) {
  paint_.setColor(color);
}
void DisplayListCanvasDispatcher::setBlendMode(SkBlendMode mode) {
  paint_.setBlendMode(mode);
}
void DisplayListCanvasDispatcher::setFilterQuality(SkFilterQuality quality) {
  paint_.setFilterQuality(quality);
  switch (quality) {
    case SkFilterQuality::kNone_SkFilterQuality:
      sampling_ = DisplayList::NearestSampling;
      filtering_ = SkFilterMode::kNearest;
      break;
    case SkFilterQuality::kLow_SkFilterQuality:
      sampling_ = DisplayList::LinearSampling;
      filtering_ = SkFilterMode::kLinear;
      break;
    case SkFilterQuality::kMedium_SkFilterQuality:
      sampling_ = DisplayList::MipmapSampling;
      filtering_ = SkFilterMode::kLinear;
      break;
    case SkFilterQuality::kHigh_SkFilterQuality:
      sampling_ = DisplayList::CubicSampling;
      filtering_ = SkFilterMode::kLinear;
      break;
    default:
      FML_DCHECK(0);
      return;
  }
}
void DisplayListCanvasDispatcher::setShader(sk_sp<SkShader> shader) {
  paint_.setShader(shader);
}
void DisplayListCanvasDispatcher::setImageFilter(sk_sp<SkImageFilter> filter) {
  paint_.setImageFilter(filter);
}
void DisplayListCanvasDispatcher::setColorFilter(sk_sp<SkColorFilter> filter) {
  color_filter_ = filter;
  paint_.setColorFilter(makeColorFilter());
}
void DisplayListCanvasDispatcher::setMaskFilter(sk_sp<SkMaskFilter> filter) {
  paint_.setMaskFilter(filter);
}

void DisplayListCanvasDispatcher::save() {
  canvas_->save();
}
void DisplayListCanvasDispatcher::restore() {
  canvas_->restore();
}
void DisplayListCanvasDispatcher::saveLayer(const SkRect* bounds) {
  canvas_->saveLayer(bounds, &paint_);
}

void DisplayListCanvasDispatcher::translate(SkScalar tx, SkScalar ty) {
  canvas_->translate(tx, ty);
}
void DisplayListCanvasDispatcher::scale(SkScalar sx, SkScalar sy) {
  canvas_->scale(sx, sy);
}
void DisplayListCanvasDispatcher::rotate(SkScalar degrees) {
  canvas_->rotate(degrees);
}
void DisplayListCanvasDispatcher::skew(SkScalar sx, SkScalar sy) {
  canvas_->skew(sx, sy);
}
void DisplayListCanvasDispatcher::transform2x3(SkScalar mxx,
                                               SkScalar mxy,
                                               SkScalar mxt,
                                               SkScalar myx,
                                               SkScalar myy,
                                               SkScalar myt) {
  canvas_->concat(SkMatrix::MakeAll(mxx, mxy, mxt, myx, myy, myt, 0, 0, 1));
}
void DisplayListCanvasDispatcher::transform3x3(SkScalar mxx,
                                               SkScalar mxy,
                                               SkScalar mxt,
                                               SkScalar myx,
                                               SkScalar myy,
                                               SkScalar myt,
                                               SkScalar px,
                                               SkScalar py,
                                               SkScalar pt) {
  canvas_->concat(SkMatrix::MakeAll(mxx, mxy, mxt, myx, myy, myt, px, py, pt));
}

void DisplayListCanvasDispatcher::clipRect(const SkRect& rect,
                                           bool isAA,
                                           SkClipOp clip_op) {
  canvas_->clipRect(rect, clip_op, isAA);
}
void DisplayListCanvasDispatcher::clipRRect(const SkRRect& rrect, bool isAA) {
  canvas_->clipRRect(rrect, isAA);
}
void DisplayListCanvasDispatcher::clipPath(const SkPath& path, bool isAA) {
  canvas_->clipPath(path, isAA);
}

void DisplayListCanvasDispatcher::drawPaint() {
  canvas_->drawPaint(paint_);
}
void DisplayListCanvasDispatcher::drawColor(SkColor color, SkBlendMode mode) {
  canvas_->drawColor(color, mode);
}
void DisplayListCanvasDispatcher::drawLine(const SkPoint& p0,
                                           const SkPoint& p1) {
  canvas_->drawLine(p0, p1, paint_);
}
void DisplayListCanvasDispatcher::drawRect(const SkRect& rect) {
  canvas_->drawRect(rect, paint_);
}
void DisplayListCanvasDispatcher::drawOval(const SkRect& bounds) {
  canvas_->drawOval(bounds, paint_);
}
void DisplayListCanvasDispatcher::drawCircle(const SkPoint& center,
                                             SkScalar radius) {
  canvas_->drawCircle(center, radius, paint_);
}
void DisplayListCanvasDispatcher::drawRRect(const SkRRect& rrect) {
  canvas_->drawRRect(rrect, paint_);
}
void DisplayListCanvasDispatcher::drawDRRect(const SkRRect& outer,
                                             const SkRRect& inner) {
  canvas_->drawDRRect(outer, inner, paint_);
}
void DisplayListCanvasDispatcher::drawPath(const SkPath& path) {
  canvas_->drawPath(path, paint_);
}
void DisplayListCanvasDispatcher::drawArc(const SkRect& bounds,
                                          SkScalar start,
                                          SkScalar sweep,
                                          bool useCenter) {
  canvas_->drawArc(bounds, start, sweep, useCenter, paint_);
}
void DisplayListCanvasDispatcher::drawPoints(SkCanvas::PointMode mode,
                                             size_t count,
                                             const SkPoint pts[]) {
  canvas_->drawPoints(mode, count, pts, paint_);
}
void DisplayListCanvasDispatcher::drawVertices(const sk_sp<SkVertices> vertices,
                                               SkBlendMode mode) {
  canvas_->drawVertices(vertices, paint_);
}
void DisplayListCanvasDispatcher::drawImage(const sk_sp<SkImage> image,
                                            const SkPoint point) {
  canvas_->drawImage(image, point.fX, point.fY, sampling_, &paint_);
}
void DisplayListCanvasDispatcher::drawImageRect(const sk_sp<SkImage> image,
                                                const SkRect& src,
                                                const SkRect& dst) {
  canvas_->drawImageRect(image, dst, sampling_, &paint_);
}
void DisplayListCanvasDispatcher::drawImageNine(const sk_sp<SkImage> image,
                                                const SkRect& center,
                                                const SkRect& dst) {
  canvas_->drawImageNine(image.get(), center.round(), dst, filtering_, &paint_);
}
void DisplayListCanvasDispatcher::drawAtlas(const sk_sp<SkImage> atlas,
                                            const SkRSXform xform[],
                                            const SkRect tex[],
                                            const SkColor colors[],
                                            int count,
                                            SkBlendMode mode,
                                            const SkRect* cullRect) {
  canvas_->drawAtlas(atlas.get(), xform, tex, colors, count, mode, sampling_,
                     cullRect, &paint_);
}
void DisplayListCanvasDispatcher::drawPicture(const sk_sp<SkPicture> picture) {
  canvas_->drawPicture(picture);
}
void DisplayListCanvasDispatcher::drawDisplayList(
    const sk_sp<DisplayList> display_list) {
  int save_count = canvas_->save();
  {
    DisplayListCanvasDispatcher dispatcher(canvas_);
    display_list->dispatch(dispatcher);
  }
  canvas_->restoreToCount(save_count);
}
void DisplayListCanvasDispatcher::drawShadow(const SkPath& path,
                                             const SkColor color,
                                             const SkScalar elevation,
                                             bool occludes) {
  flutter::PhysicalShapeLayer::DrawShadow(canvas_, path, color, elevation,
                                          occludes, 1.0);
}

sk_sp<SkColorFilter> DisplayListCanvasDispatcher::makeColorFilter() {
  if (!invert_colors_) {
    return color_filter_;
  }
  sk_sp<SkColorFilter> invert_filter =
      SkColorFilters::Matrix(invert_color_matrix);
  if (color_filter_) {
    invert_filter = invert_filter->makeComposed(color_filter_);
  }
  return invert_filter;
}

DisplayListCanvasRecorder::DisplayListCanvasRecorder(const SkRect& bounds)
    : SkCanvasVirtualEnforcer(bounds.width(), bounds.height()) {}

void DisplayListCanvasRecorder::didConcat44(const SkM44& m44) {
  SkMatrix m = m44.asM33();
  if (m.hasPerspective()) {
    builder_.transform3x3(m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8]);
  } else {
    builder_.transform2x3(m[0], m[1], m[2], m[3], m[4], m[5]);
  }
}
void DisplayListCanvasRecorder::didTranslate(SkScalar tx, SkScalar ty) {
  builder_.translate(tx, ty);
}
void DisplayListCanvasRecorder::didScale(SkScalar sx, SkScalar sy) {
  builder_.scale(sx, sy);
}

void DisplayListCanvasRecorder::onClipRect(const SkRect& rect,
                                           SkClipOp op,
                                           ClipEdgeStyle edgeStyle) {
  builder_.clipRect(rect, edgeStyle == ClipEdgeStyle::kSoft_ClipEdgeStyle, op);
}
void DisplayListCanvasRecorder::onClipRRect(const SkRRect& rrect,
                                            SkClipOp op,
                                            ClipEdgeStyle edgeStyle) {
  FML_DCHECK(op == SkClipOp::kIntersect);
  builder_.clipRRect(rrect, edgeStyle == ClipEdgeStyle::kSoft_ClipEdgeStyle);
}
void DisplayListCanvasRecorder::onClipPath(const SkPath& path,
                                           SkClipOp op,
                                           ClipEdgeStyle edgeStyle) {
  FML_DCHECK(op == SkClipOp::kIntersect);
  builder_.clipPath(path, edgeStyle == ClipEdgeStyle::kSoft_ClipEdgeStyle);
}

void DisplayListCanvasRecorder::willSave() {
  builder_.save();
}
SkCanvas::SaveLayerStrategy DisplayListCanvasRecorder::getSaveLayerStrategy(
    const SaveLayerRec& rec) {
  builder_.saveLayer(rec.fBounds);
  return SaveLayerStrategy::kNoLayer_SaveLayerStrategy;
}
void DisplayListCanvasRecorder::didRestore() {
  builder_.restore();
}

void DisplayListCanvasRecorder::onDrawPaint(const SkPaint& paint) {
  recordPaintAttributes(paint, paintMask_);
  builder_.drawPaint();
}
void DisplayListCanvasRecorder::onDrawRect(const SkRect& rect,
                                           const SkPaint& paint) {
  recordPaintAttributes(paint, drawMask_);
  builder_.drawRect(rect);
}
void DisplayListCanvasRecorder::onDrawRRect(const SkRRect& rrect,
                                            const SkPaint& paint) {
  recordPaintAttributes(paint, drawMask_);
  builder_.drawRRect(rrect);
}
void DisplayListCanvasRecorder::onDrawDRRect(const SkRRect& outer,
                                             const SkRRect& inner,
                                             const SkPaint& paint) {
  recordPaintAttributes(paint, drawMask_);
  builder_.drawDRRect(outer, inner);
}
void DisplayListCanvasRecorder::onDrawOval(const SkRect& rect,
                                           const SkPaint& paint) {
  recordPaintAttributes(paint, drawMask_);
  builder_.drawOval(rect);
}
void DisplayListCanvasRecorder::onDrawArc(const SkRect& rect,
                                          SkScalar startAngle,
                                          SkScalar sweepAngle,
                                          bool useCenter,
                                          const SkPaint& paint) {
  recordPaintAttributes(paint, drawMask_);
  builder_.drawArc(rect, startAngle, sweepAngle, useCenter);
}
void DisplayListCanvasRecorder::onDrawPath(const SkPath& path,
                                           const SkPaint& paint) {
  recordPaintAttributes(paint, drawMask_);
  builder_.drawPath(path);
}

void DisplayListCanvasRecorder::onDrawPoints(SkCanvas::PointMode mode,
                                             size_t count,
                                             const SkPoint pts[],
                                             const SkPaint& paint) {
  recordPaintAttributes(paint, strokeMask_);
  if (mode == SkCanvas::PointMode::kLines_PointMode && count == 2) {
    builder_.drawLine(pts[0], pts[1]);
  } else {
    builder_.drawPoints(mode, count, pts);
  }
}
void DisplayListCanvasRecorder::onDrawVerticesObject(const SkVertices* vertices,
                                                     SkBlendMode mode,
                                                     const SkPaint& paint) {
  recordPaintAttributes(paint, drawMask_);
  builder_.drawVertices(sk_ref_sp(vertices), mode);
}

void DisplayListCanvasRecorder::onDrawImage2(const SkImage* image,
                                             SkScalar dx,
                                             SkScalar dy,
                                             const SkSamplingOptions& sampling,
                                             const SkPaint* paint) {
  if (paint) {
    recordPaintAttributes(*paint, imageMask_);
  } else {
    recordPaintAttributes(SkPaint(), imageMask_);
  }
  // TODO: Update Sampling
  builder_.drawImage(sk_ref_sp(image), SkPoint::Make(dx, dy));
}
void DisplayListCanvasRecorder::onDrawImageRect2(
    const SkImage* image,
    const SkRect& src,
    const SkRect& dst,
    const SkSamplingOptions& sampling,
    const SkPaint* paint,
    SrcRectConstraint constraint) {
  FML_DCHECK(constraint == SrcRectConstraint::kFast_SrcRectConstraint);
  if (paint) {
    recordPaintAttributes(*paint, imageMask_);
  } else {
    recordPaintAttributes(SkPaint(), imageMask_);
  }
  // TODO: Update Sampling
  builder_.drawImageRect(sk_ref_sp(image), src, dst);
}
void DisplayListCanvasRecorder::onDrawAtlas2(const SkImage* image,
                                             const SkRSXform xform[],
                                             const SkRect src[],
                                             const SkColor colors[],
                                             int count,
                                             SkBlendMode mode,
                                             const SkSamplingOptions& sampling,
                                             const SkRect* cull,
                                             const SkPaint* paint) {
  if (paint) {
    recordPaintAttributes(*paint, imageMask_);
  } else {
    recordPaintAttributes(SkPaint(), imageMask_);
  }
  // TODO: Update Sampling
  builder_.drawAtlas(sk_ref_sp(image), xform, src, colors, count, mode, cull);
}

void DisplayListCanvasRecorder::onDrawShadowRec(const SkPath& path,
                                                const SkDrawShadowRec& rec) {
  FML_LOG(ERROR) << "Ignoring shadow";
}

void DisplayListCanvasRecorder::onDrawPicture(const SkPicture* picture,
                                              const SkMatrix* matrix,
                                              const SkPaint* paint) {
  FML_DCHECK(matrix == nullptr);
  FML_DCHECK(paint == nullptr);
  builder_.drawPicture(sk_ref_sp(picture));
}

void DisplayListCanvasRecorder::recordPaintAttributes(const SkPaint& paint,
                                                      int dataNeeded) {
  if ((dataNeeded & aaNeeded_) != 0 && currentAA_ != paint.isAntiAlias()) {
    builder_.setAA(currentAA_ = paint.isAntiAlias());
  }
  if ((dataNeeded & ditherNeeded_) != 0 && currentDither_ != paint.isDither()) {
    builder_.setDither(currentDither_ = paint.isDither());
  }
  if ((dataNeeded & colorNeeded_) != 0 && currentColor_ != paint.getColor()) {
    builder_.setColor(currentColor_ = paint.getColor());
  }
  if ((dataNeeded & blendNeeded_) != 0 &&
      currentBlendMode_ != paint.getBlendMode()) {
    builder_.setBlendMode(currentBlendMode_ = paint.getBlendMode());
  }
  // invert colors is a Flutter::Paint thing, not an SkPaint thing
  // if ((dataNeeded & invertColorsNeeded_) != 0 &&
  //     currentInvertColors_ != paint.???) {
  //   currentInvertColors_ = paint.invertColors;
  //   addOp_(currentInvertColors_
  //          ? _CanvasOp.setInvertColors
  //          : _CanvasOp.clearInvertColors, 0);
  // }
  if ((dataNeeded & paintStyleNeeded_) != 0) {
    if (currentPaintStyle_ != paint.getStyle()) {
      FML_DCHECK(paint.getStyle() != SkPaint::kStrokeAndFill_Style);
      builder_.setDrawStyle(currentPaintStyle_ = paint.getStyle());
    }
    if (currentPaintStyle_ == SkPaint::Style::kStroke_Style) {
      dataNeeded |= strokeStyleNeeded_;
    }
  }
  if ((dataNeeded & strokeStyleNeeded_) != 0) {
    if (currentStrokeWidth_ != paint.getStrokeWidth()) {
      builder_.setStrokeWidth(currentStrokeWidth_ = paint.getStrokeWidth());
    }
    if (currentStrokeCap_ != paint.getStrokeCap()) {
      builder_.setCap(currentStrokeCap_ = paint.getStrokeCap());
    }
    if (currentStrokeJoin_ != paint.getStrokeJoin()) {
      builder_.setJoin(currentStrokeJoin_ = paint.getStrokeJoin());
    }
    if (currentMiterLimit_ != paint.getStrokeMiter()) {
      builder_.setMiterLimit(currentMiterLimit_ = paint.getStrokeMiter());
    }
  }
  if ((dataNeeded & filterQualityNeeded_) != 0 &&
      currentFilterQuality_ != paint.getFilterQuality()) {
    builder_.setFilterQuality(currentFilterQuality_ = paint.getFilterQuality());
  }
  if ((dataNeeded & shaderNeeded_) != 0 &&
      currentShader_.get() != paint.getShader()) {
    builder_.setShader(currentShader_ = sk_ref_sp(paint.getShader()));
  }
  if ((dataNeeded & colorFilterNeeded_) != 0 &&
      currentColorFilter_.get() != paint.getColorFilter()) {
    builder_.setColorFilter(currentColorFilter_ =
                                sk_ref_sp(paint.getColorFilter()));
  }
  if ((dataNeeded & imageFilterNeeded_) != 0 &&
      currentImageFilter_.get() != paint.getImageFilter()) {
    builder_.setImageFilter(currentImageFilter_ =
                                sk_ref_sp(paint.getImageFilter()));
  }
  if ((dataNeeded & maskFilterNeeded_) != 0 &&
      currentMaskFilter_.get() != paint.getMaskFilter()) {
    builder_.setMaskFilter(currentMaskFilter_ =
                               sk_ref_sp(paint.getMaskFilter()));
  }
}

}  // namespace flutter
