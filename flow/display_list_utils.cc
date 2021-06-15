// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <type_traits>

#include "flutter/flow/display_list_utils.h"
#include "flutter/fml/logging.h"

#include "third_party/skia/include/core/SkMaskFilter.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkRSXform.h"

namespace flutter {

void SkMatrixTransformDispatchHelper::translate(SkScalar tx, SkScalar ty) {
  matrix_.preTranslate(tx, ty);
}
void SkMatrixTransformDispatchHelper::scale(SkScalar sx, SkScalar sy) {
  matrix_.preScale(sx, sy);
}
void SkMatrixTransformDispatchHelper::rotate(SkScalar degrees) {
  matrix_.preRotate(degrees);
}
void SkMatrixTransformDispatchHelper::skew(SkScalar sx, SkScalar sy) {
  matrix_.preSkew(sx, sy);
}
void SkMatrixTransformDispatchHelper::transform2x3(SkScalar mxx,
                                                   SkScalar mxy,
                                                   SkScalar mxt,
                                                   SkScalar myx,
                                                   SkScalar myy,
                                                   SkScalar myt) {
  matrix_.preConcat(SkMatrix::MakeAll(mxx, mxy, mxt, myx, myy, myt, 0, 0, 1));
}
void SkMatrixTransformDispatchHelper::transform3x3(SkScalar mxx,
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
void SkMatrixTransformDispatchHelper::save() {
  saved_.push_back(matrix_);
}
void SkMatrixTransformDispatchHelper::restore() {
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
  SkRect devClipBounds = txSource_->getMatrix().mapRect(rect);
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
}

void DisplayListBoundsCalculator::saveLayer(const SkRect* bounds) {}
void DisplayListBoundsCalculator::save() {}
void DisplayListBoundsCalculator::restore() {}

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
  if (count > 1) {
    BoundsAccumulator ptBounds;
    for (size_t i = 0; i < count; i++) {
      ptBounds.accumulate(pts[i]);
    }
    if (ptBounds.isNotEmpty()) {
      accumulateRect(ptBounds.getBounds(), GEOM_STROKE);
    }
  }
}
void DisplayListBoundsCalculator::drawVertices(const sk_sp<SkVertices> vertices,
                                               SkBlendMode mode) {
  accumulateRect(vertices->bounds());
}
void DisplayListBoundsCalculator::drawImage(const sk_sp<SkImage> image,
                                            const SkPoint point) {
  SkRect bounds = SkRect::Make(image->bounds());
  bounds.offset(point);
  accumulateRect(bounds, NON_GEOM);
}
void DisplayListBoundsCalculator::drawImageRect(const sk_sp<SkImage> image,
                                                const SkRect& src,
                                                const SkRect& dst) {
  accumulateRect(dst, NON_GEOM);
}
void DisplayListBoundsCalculator::drawImageNine(const sk_sp<SkImage> image,
                                                const SkRect& center,
                                                const SkRect& dst) {
  accumulateRect(dst, NON_GEOM);
}
void DisplayListBoundsCalculator::drawAtlas(const sk_sp<SkImage> atlas,
                                            const SkRSXform xform[],
                                            const SkRect tex[],
                                            const SkColor colors[],
                                            int count,
                                            SkBlendMode mode,
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
void DisplayListBoundsCalculator::drawShadow(const SkPath& path,
                                             const SkColor color,
                                             const SkScalar elevation,
                                             bool occludes) {
  // TODO(flar)
}
void DisplayListBoundsCalculator::accumulatePoint(const SkPoint& p,
                                                  BoundsType type) {
  if (type == GEOM_FILL && !maskFilter_ && !imageFilter_) {
    SkPoint pDst;
    getMatrix().mapPoints(&pDst, &p, 1);
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
  if (type != NON_GEOM && maskFilter_) {
    // Just using bondsPaint for its bounds computation skills
    // If we capture mask filters as the blur sigmas instead
    // of hiding them inside an uninspectable SkMaskFilter,
    // we could compute the bounds directly.
    SkPaint boundsPaint;
    boundsPaint.setMaskFilter(maskFilter_);
    boundsPaint.setStyle(SkPaint::Style::kFill_Style);
    FML_DCHECK(boundsPaint.canComputeFastBounds());
    dstRect = boundsPaint.computeFastBounds(dstRect, &dstRect);
  }
  if (imageFilter_) {
    SkIRect outBounds =
        imageFilter_->filterBounds(dstRect.roundOut(), SkMatrix::I(),
                                   SkImageFilter::kForward_MapDirection);
    dstRect.set(outBounds);
  }
  getMatrix().mapRect(dstRect);
  accumulator_.accumulate(dstRect.fLeft, dstRect.fTop);
  accumulator_.accumulate(dstRect.fRight, dstRect.fBottom);
}

}  // namespace flutter
