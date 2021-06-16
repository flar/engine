// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <type_traits>

#include "flutter/flow/display_list_utils.h"
#include "flutter/flow/layers/physical_shape_layer.h"
#include "flutter/fml/logging.h"

#include "third_party/skia/include/core/SkMaskFilter.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkRSXform.h"
#include "third_party/skia/include/core/SkTextBlob.h"
#include "third_party/skia/include/utils/SkShadowUtils.h"

// This header file cannot be included here, but we cannot
// record calls made by the SkShadowUtils without it.
// #include "third_party/skia/src/core/SkDrawShadowInfo.h"

namespace flutter {

// clang-format off
constexpr float invert_color_matrix[20] = {
  -1.0,    0,    0, 1.0, 0,
     0, -1.0,    0, 1.0, 0,
     0,    0, -1.0, 1.0, 0,
   1.0,  1.0,  1.0, 1.0, 0
};
// clang-format on

void SkPaintDispatchHelper::setAA(bool aa) {
  paint_.setAntiAlias(true);
}
void SkPaintDispatchHelper::setDither(bool dither) {
  paint_.setAntiAlias(false);
}
void SkPaintDispatchHelper::setInvertColors(bool invert) {
  invert_colors_ = invert;
  paint_.setColorFilter(makeColorFilter());
}
void SkPaintDispatchHelper::setCap(SkPaint::Cap cap) {
  paint_.setStrokeCap(cap);
}
void SkPaintDispatchHelper::setJoin(SkPaint::Join join) {
  paint_.setStrokeJoin(join);
}
void SkPaintDispatchHelper::setDrawStyle(SkPaint::Style style) {
  paint_.setStyle(style);
}
void SkPaintDispatchHelper::setStrokeWidth(SkScalar width) {
  paint_.setStrokeWidth(width);
}
void SkPaintDispatchHelper::setMiterLimit(SkScalar limit) {
  paint_.setStrokeMiter(limit);
}
void SkPaintDispatchHelper::setColor(SkColor color) {
  paint_.setColor(color);
}
void SkPaintDispatchHelper::setBlendMode(SkBlendMode mode) {
  paint_.setBlendMode(mode);
}
void SkPaintDispatchHelper::setFilterQuality(SkFilterQuality quality) {
  paint_.setFilterQuality(quality);
}
void SkPaintDispatchHelper::setShader(sk_sp<SkShader> shader) {
  paint_.setShader(shader);
}
void SkPaintDispatchHelper::setImageFilter(sk_sp<SkImageFilter> filter) {
  paint_.setImageFilter(filter);
}
void SkPaintDispatchHelper::setColorFilter(sk_sp<SkColorFilter> filter) {
  color_filter_ = filter;
  paint_.setColorFilter(makeColorFilter());
}
void SkPaintDispatchHelper::setMaskFilter(sk_sp<SkMaskFilter> filter) {
  paint_.setMaskFilter(filter);
}
void SkPaintDispatchHelper::setMaskBlurFilter(SkBlurStyle style,
                                              SkScalar sigma) {
  paint_.setMaskFilter(SkMaskFilter::MakeBlur(style, sigma));
}

sk_sp<SkColorFilter> SkPaintDispatchHelper::makeColorFilter() {
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

void SkMatrixDispatchHelper::translate(SkScalar tx, SkScalar ty) {
  matrix_.preTranslate(tx, ty);
}
void SkMatrixDispatchHelper::scale(SkScalar sx, SkScalar sy) {
  matrix_.preScale(sx, sy);
}
void SkMatrixDispatchHelper::rotate(SkScalar degrees) {
  matrix_.preRotate(degrees);
}
void SkMatrixDispatchHelper::skew(SkScalar sx, SkScalar sy) {
  matrix_.preSkew(sx, sy);
}
void SkMatrixDispatchHelper::transform2x3(SkScalar mxx,
                                          SkScalar mxy,
                                          SkScalar mxt,
                                          SkScalar myx,
                                          SkScalar myy,
                                          SkScalar myt) {
  matrix_.preConcat(SkMatrix::MakeAll(mxx, mxy, mxt, myx, myy, myt, 0, 0, 1));
}
void SkMatrixDispatchHelper::transform3x3(SkScalar mxx,
                                          SkScalar mxy,
                                          SkScalar mxt,
                                          SkScalar myx,
                                          SkScalar myy,
                                          SkScalar myt,
                                          SkScalar px,
                                          SkScalar py,
                                          SkScalar pt) {
  matrix_.preConcat(
      SkMatrix::MakeAll(mxx, mxy, mxt, myx, myy, myt, px, py, pt));
}
void SkMatrixDispatchHelper::save() {
  saved_.push_back(matrix_);
}
void SkMatrixDispatchHelper::restore() {
  matrix_ = saved_.back();
  saved_.pop_back();
}

void ClipBoundsDispatchHelper::clipRect(const SkRect& rect,
                                        bool isAA,
                                        SkClipOp clip_op) {
  if (clip_op == SkClipOp::kIntersect) {
    intersect(rect);
  }
}
void ClipBoundsDispatchHelper::clipRRect(const SkRRect& rrect, bool isAA) {
  intersect(rrect.getBounds());
}
void ClipBoundsDispatchHelper::clipPath(const SkPath& path, bool isAA) {
  intersect(path.getBounds());
}
void ClipBoundsDispatchHelper::intersect(const SkRect& rect) {
  SkRect devClipBounds = matrix().mapRect(rect);
  if (!bounds_.intersect(devClipBounds)) {
    bounds_.setEmpty();
  }
}
void ClipBoundsDispatchHelper::save() {
  saved_.push_back(bounds_);
}
void ClipBoundsDispatchHelper::restore() {
  bounds_ = saved_.back();
  saved_.pop_back();
}

void DisplayListBoundsCalculator::setJoin(SkPaint::Join join) {
  isMiter_ = (join == SkPaint::Join::kMiter_Join);
}
void DisplayListBoundsCalculator::setDrawStyle(SkPaint::Style style) {
  switch (style) {
    case SkPaint::kFill_Style:
      geomType_ = GEOM_FILL;
      break;
    case SkPaint::kStroke_Style:
    case SkPaint::kStrokeAndFill_Style:
      geomType_ = GEOM_STROKE;
      break;
  }
}
void DisplayListBoundsCalculator::setStrokeWidth(SkScalar width) {
  strokeWidth_ = width;
}
void DisplayListBoundsCalculator::setMiterLimit(SkScalar limit) {
  miterLimit_ = limit;
}
void DisplayListBoundsCalculator::setImageFilter(sk_sp<SkImageFilter> filter) {
  imageFilter_ = filter;
}
void DisplayListBoundsCalculator::setMaskFilter(sk_sp<SkMaskFilter> filter) {
  maskFilter_ = filter;
  maskBlurSigma_ = 0.0;
}
void DisplayListBoundsCalculator::setMaskBlurFilter(SkBlurStyle style,
                                                    SkScalar sigma) {
  maskFilter_ = nullptr;
  maskBlurStyle_ = style;
  maskBlurSigma_ = sigma;
}

void DisplayListBoundsCalculator::saveLayer(const SkRect* bounds) {
  save();
}
void DisplayListBoundsCalculator::save() {
  SkMatrixDispatchHelper::save();
  ClipBoundsDispatchHelper::save();
}
void DisplayListBoundsCalculator::restore() {
  SkMatrixDispatchHelper::restore();
  ClipBoundsDispatchHelper::restore();
}

void DisplayListBoundsCalculator::drawPaint() {
  // Paints entire surface, doesn't really affect computed bounds;
}
void DisplayListBoundsCalculator::drawColor(SkColor color, SkBlendMode mode) {
  // Paints entire surface, doesn't really affect computed bounds;
}
void DisplayListBoundsCalculator::drawLine(const SkPoint& p0,
                                           const SkPoint& p1) {
  accumulatePoint(p0, GEOM_STROKE);
  accumulatePoint(p1, GEOM_STROKE);
}
void DisplayListBoundsCalculator::drawRect(const SkRect& rect) {
  accumulateRect(rect);
}
void DisplayListBoundsCalculator::drawOval(const SkRect& bounds) {
  accumulateRect(bounds);
}
void DisplayListBoundsCalculator::drawCircle(const SkPoint& center,
                                             SkScalar radius) {
  accumulateRect(SkRect::MakeLTRB(center.fX - radius, center.fY - radius,
                                  center.fX + radius, center.fY + radius));
}
void DisplayListBoundsCalculator::drawRRect(const SkRRect& rrect) {
  accumulateRect(rrect.getBounds());
}
void DisplayListBoundsCalculator::drawDRRect(const SkRRect& outer,
                                             const SkRRect& inner) {
  accumulateRect(outer.getBounds());
}
void DisplayListBoundsCalculator::drawPath(const SkPath& path) {
  accumulateRect(path.getBounds());
}
void DisplayListBoundsCalculator::drawArc(const SkRect& bounds,
                                          SkScalar start,
                                          SkScalar sweep,
                                          bool useCenter) {
  // This could be tighter if we compute where the start and end
  // angles are and then also consider the quadrants swept and
  // the center if specified.
  accumulateRect(bounds);
}
void DisplayListBoundsCalculator::drawPoints(SkCanvas::PointMode mode,
                                             size_t count,
                                             const SkPoint pts[]) {
  if (count > 0) {
    BoundsAccumulator ptBounds;
    for (size_t i = 0; i < count; i++) {
      ptBounds.accumulate(pts[i]);
    }
    accumulateRect(ptBounds.getBounds(), GEOM_STROKE);
  }
}
void DisplayListBoundsCalculator::drawVertices(const sk_sp<SkVertices> vertices,
                                               SkBlendMode mode) {
  accumulateRect(vertices->bounds());
}
void DisplayListBoundsCalculator::drawImage(const sk_sp<SkImage> image,
                                            const SkPoint point,
                                            const SkSamplingOptions& sampling) {
  SkRect bounds = SkRect::Make(image->bounds());
  bounds.offset(point);
  accumulateRect(bounds, NON_GEOM);
}
void DisplayListBoundsCalculator::drawImageRect(
    const sk_sp<SkImage> image,
    const SkRect& src,
    const SkRect& dst,
    const SkSamplingOptions& sampling) {
  accumulateRect(dst, NON_GEOM);
}
void DisplayListBoundsCalculator::drawImageNine(const sk_sp<SkImage> image,
                                                const SkRect& center,
                                                const SkRect& dst,
                                                SkFilterMode filter) {
  accumulateRect(dst, NON_GEOM);
}
void DisplayListBoundsCalculator::drawImageLattice(
    const sk_sp<SkImage> image,
    const SkCanvas::Lattice& lattice,
    const SkRect& dst,
    SkFilterMode filter) {
  accumulateRect(dst, NON_GEOM);
}
void DisplayListBoundsCalculator::drawAtlas(const sk_sp<SkImage> atlas,
                                            const SkRSXform xform[],
                                            const SkRect tex[],
                                            const SkColor colors[],
                                            int count,
                                            SkBlendMode mode,
                                            const SkSamplingOptions& sampling,
                                            const SkRect* cullRect) {
  SkPoint quad[4];
  BoundsAccumulator atlasBounds;
  for (int i = 0; i < count; i++) {
    const SkRect& src = tex[i];
    xform[i].toQuad(src.width(), src.height(), quad);
    for (int j = 0; j < 4; j++) {
      atlasBounds.accumulate(quad[j]);
    }
  }
  if (atlasBounds.isNotEmpty()) {
    accumulateRect(atlasBounds.getBounds(), NON_GEOM);
  }
}
void DisplayListBoundsCalculator::drawPicture(const sk_sp<SkPicture> picture) {
  // TODO(flar) cull rect really cannot be trusted in general, but
  // it will work for SkPictures generated from our own PictureRecorder.
  accumulateRect(picture->cullRect());
}
void DisplayListBoundsCalculator::drawDisplayList(
    const sk_sp<DisplayList> display_list) {
  accumulateRect(display_list->bounds());
}
void DisplayListBoundsCalculator::drawTextBlob(const sk_sp<SkTextBlob> blob,
                                               SkScalar x,
                                               SkScalar y) {
  accumulateRect(blob->bounds().makeOffset(x, y));
}
// void DisplayListBoundsCalculator::drawShadowRec(const SkPath& path,
//                                                 const SkDrawShadowRec& rec) {
//   SkRect bounds;
//   SkDrawShadowMetrics::GetLocalBounds(path, rec, SkMatrix::I(), &bounds);
//   accumulateRect(bounds, NON_GEOM);
// }
void DisplayListBoundsCalculator::drawShadow(const SkPath& path,
                                             const SkColor color,
                                             const SkScalar elevation,
                                             bool occludes) {
  SkRect bounds =
      PhysicalShapeLayer::ComputeShadowBounds(path.getBounds(), elevation, 1.0);
  accumulateRect(bounds, NON_GEOM);
}
void DisplayListBoundsCalculator::accumulatePoint(const SkPoint& p,
                                                  BoundsType type) {
  if (type == GEOM_FILL && !maskFilter_ && !imageFilter_) {
    SkPoint pDst;
    matrix().mapPoints(&pDst, &p, 1);
    accumulator_.accumulate(pDst);
    return;
  }
  accumulateRect(SkRect::MakeXYWH(p.fX, p.fY, 0, 0), type);
}
void DisplayListBoundsCalculator::accumulateRect(const SkRect& rect,
                                                 BoundsType type) {
  SkRect dstRect = rect;
  if (type == GEOM_STROKE) {
    SkScalar pad = strokeWidth_ * 0.5;
    if (isMiter_)
      pad *= miterLimit_;
    dstRect.outset(pad, pad);
  }
  if (type != NON_GEOM) {
    if (maskFilter_) {
      // Just using bondsPaint for its bounds computation skills
      // If we capture mask filters as the blur sigmas instead
      // of hiding them inside an uninspectable SkMaskFilter,
      // we could compute the bounds directly.
      SkPaint boundsPaint;
      boundsPaint.setMaskFilter(maskFilter_);
      boundsPaint.setStyle(SkPaint::Style::kFill_Style);
      FML_DCHECK(boundsPaint.canComputeFastBounds());
      dstRect = boundsPaint.computeFastBounds(dstRect, &dstRect);
    } else if (maskBlurSigma_ > 0) {
      dstRect.outset(3.0 * maskBlurSigma_, 3.0 * maskBlurSigma_);
    }
  }
  if (imageFilter_) {
    SkIRect outBounds =
        imageFilter_->filterBounds(dstRect.roundOut(), SkMatrix::I(),
                                   SkImageFilter::kForward_MapDirection);
    dstRect.set(outBounds);
  }
  matrix().mapRect(dstRect);
  accumulator_.accumulate(dstRect.fLeft, dstRect.fTop);
  accumulator_.accumulate(dstRect.fRight, dstRect.fBottom);
}

}  // namespace flutter
