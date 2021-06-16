// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_FLOW_DISPLAY_LIST_UTILS_H_
#define FLUTTER_FLOW_DISPLAY_LIST_UTILS_H_

#include "flutter/flow/display_list.h"

#include "third_party/skia/include/core/SkMaskFilter.h"

// This file contains various utility classes to ease implementing
// a Flutter DisplayList Dispatcher, including:
//
// IngoreAttributeDispatchHelper:
// IngoreClipDispatchHelper:
// IngoreTransformDispatchHelper
//     Empty overrides of all of the associated methods of Dispatcher
//     for dispatchers that only track some of the rendering operations
//
// SkPaintAttributeDispatchHelper:
//     Tracks the attribute methods and maintains their state in an
//     SkPaint object.
// SkMatrixTransformDispatchHelper:
//     Tracks the transform methods and maintains their state in a
//     (save/restore stack of) SkMatrix object.
// ClipBoundsDispatchHelper:
//     Tracks the clip methods and maintains a culling box in a
//     (save/restore stack of) SkRect culling rectangle.
//
// DisplayListBoundsCalculator:
//     A class that can traverse an entire display list and compute
//     a conservative estimate of the bounds of all of the rendering
//     operations.

namespace flutter {

// A utility class that will ignore all Dispatcher methods relating
// to the setting of attributes.
class IngoreAttributeDispatchHelper : public virtual Dispatcher {
 public:
  void setAA(bool aa) override {}
  void setDither(bool dither) override {}
  void setInvertColors(bool invert) override {}
  void setCap(SkPaint::Cap cap) override {}
  void setJoin(SkPaint::Join join) override {}
  void setDrawStyle(SkPaint::Style style) override {}
  void setStrokeWidth(SkScalar width) override {}
  void setMiterLimit(SkScalar limit) override {}
  void setColor(SkColor color) override {}
  void setBlendMode(SkBlendMode mode) override {}
  void setFilterQuality(SkFilterQuality quality) override {}
  void setShader(sk_sp<SkShader> shader) override {}
  void setImageFilter(sk_sp<SkImageFilter> filter) override {}
  void setColorFilter(sk_sp<SkColorFilter> filter) override {}
  void setMaskFilter(sk_sp<SkMaskFilter> filter) override {}
  void setMaskBlurFilter(SkBlurStyle style, SkScalar sigma) override {}
};

// A utility class that will ignore all Dispatcher methods relating
// to setting a clip.
class IngoreClipDispatchHelper : public virtual Dispatcher {
  void clipRect(const SkRect& rect, bool isAA, SkClipOp clip_op) override {}
  void clipRRect(const SkRRect& rrect, bool isAA) override {}
  void clipPath(const SkPath& path, bool isAA) override {}
};

// A utility class that will ignore all Dispatcher methods relating
// to modifying the transform.
class IngoreTransformDispatchHelper : public virtual Dispatcher {
 public:
  void translate(SkScalar tx, SkScalar ty) override {}
  void scale(SkScalar sx, SkScalar sy) override {}
  void rotate(SkScalar degrees) override {}
  void skew(SkScalar sx, SkScalar sy) override {}
  void transform2x3(SkScalar mxx,
                    SkScalar mxy,
                    SkScalar mxt,
                    SkScalar myx,
                    SkScalar myy,
                    SkScalar myt) override {}
  void transform3x3(SkScalar mxx,
                    SkScalar mxy,
                    SkScalar mxt,
                    SkScalar myx,
                    SkScalar myy,
                    SkScalar myt,
                    SkScalar px,
                    SkScalar py,
                    SkScalar pt) override {}
};

// A utility class that will monitor the Dispatcher methods relating
// to the rendering attributes and accumulate them into an SkPaint
// which can be accessed at any time via paint().
class SkPaintDispatchHelper : public virtual Dispatcher {
 public:
  void setAA(bool aa) override;
  void setDither(bool dither) override;
  void setInvertColors(bool invert) override;
  void setCap(SkPaint::Cap cap) override;
  void setJoin(SkPaint::Join join) override;
  void setDrawStyle(SkPaint::Style style) override;
  void setStrokeWidth(SkScalar width) override;
  void setMiterLimit(SkScalar limit) override;
  void setColor(SkColor color) override;
  void setBlendMode(SkBlendMode mode) override;
  void setFilterQuality(SkFilterQuality quality) override;
  void setShader(sk_sp<SkShader> shader) override;
  void setImageFilter(sk_sp<SkImageFilter> filter) override;
  void setColorFilter(sk_sp<SkColorFilter> filter) override;
  void setMaskFilter(sk_sp<SkMaskFilter> filter) override;
  void setMaskBlurFilter(SkBlurStyle style, SkScalar sigma) override;

  const SkPaint& paint() { return paint_; }

 private:
  SkPaint paint_;
  bool invert_colors_ = false;
  sk_sp<SkColorFilter> color_filter_;

  sk_sp<SkColorFilter> makeColorFilter();
};

class SkMatrixSource {
 public:
  virtual const SkMatrix& matrix() const = 0;
};

// A utility class that will monitor the Dispatcher methods relating
// to the transform and accumulate them into an SkMatrix which can
// be accessed at any time via getMatrix().
//
// This class also implements an appropriate stack of transforms via
// its save() and restore() methods so those methods will need to be
// forwarded if overridden in more than one super class.
class SkMatrixDispatchHelper : public virtual Dispatcher,
                               public virtual SkMatrixSource {
 public:
  void translate(SkScalar tx, SkScalar ty) override;
  void scale(SkScalar sx, SkScalar sy) override;
  void rotate(SkScalar degrees) override;
  void skew(SkScalar sx, SkScalar sy) override;
  void transform2x3(SkScalar mxx,
                    SkScalar mxy,
                    SkScalar mxt,
                    SkScalar myx,
                    SkScalar myy,
                    SkScalar myt) override;
  void transform3x3(SkScalar mxx,
                    SkScalar mxy,
                    SkScalar mxt,
                    SkScalar myx,
                    SkScalar myy,
                    SkScalar myt,
                    SkScalar px,
                    SkScalar py,
                    SkScalar pt) override;

  void save() override;
  void restore() override;

  const SkMatrix& matrix() const override { return matrix_; }

 private:
  SkMatrix matrix_;
  std::vector<SkMatrix> saved_;
};

// A utility class that will monitor the Dispatcher methods relating
// to the clip and accumulate a conservative bounds into an SkRect
// which can be accessed at any time via getCullingBounds().
//
// The subclass must implement a single virtual method matrix()
// which will happen automatically if the subclass also inherits
// from SkMatrixTransformDispatchHelper.
//
// This class also implements an appropriate stack of transforms via
// its save() and restore() methods so those methods will need to be
// forwarded if overridden in more than one super class.
class ClipBoundsDispatchHelper : public virtual Dispatcher,
                                 private virtual SkMatrixSource {
 public:
  void clipRect(const SkRect& rect, bool isAA, SkClipOp clip_op) override;
  void clipRRect(const SkRRect& rrect, bool isAA) override;
  void clipPath(const SkPath& path, bool isAA) override;

  void save() override;
  void restore() override;

  const SkRect& getCullingBounds() const { return bounds_; }

 private:
  SkRect bounds_;
  std::vector<SkRect> saved_;

  void intersect(const SkRect& clipBounds);
};

class BoundsAccumulator {
 public:
  void accumulate(SkPoint p) { accumulate(p.fX, p.fY); }
  void accumulate(SkScalar x, SkScalar y) {
    if (minX_ > x)
      minX_ = x;
    if (minY_ > y)
      minY_ = y;
    if (maxX_ < x)
      maxX_ = x;
    if (maxY_ < y)
      maxY_ = y;
  }

  bool isEmpty() const { return minX_ >= maxX_ || minY_ >= maxY_; }
  bool isNotEmpty() const { return minX_ < maxX_ && minY_ < maxY_; }

  SkRect getBounds() const {
    return (maxX_ > minX_ && maxY_ > minY_)
               ? SkRect::MakeLTRB(minX_, minY_, maxX_, maxY_)
               : SkRect::MakeEmpty();
  }

 private:
  SkScalar minX_ = +MAXFLOAT;
  SkScalar minY_ = +MAXFLOAT;
  SkScalar maxX_ = -MAXFLOAT;
  SkScalar maxY_ = -MAXFLOAT;
};

// This class implements all rendering methods and computes a liberal
// bounds of the rendering operations.
class DisplayListBoundsCalculator final
    : public virtual Dispatcher,
      public virtual IngoreAttributeDispatchHelper,
      public virtual SkMatrixDispatchHelper,
      public virtual ClipBoundsDispatchHelper {
 public:
  void setJoin(SkPaint::Join join) override;
  void setDrawStyle(SkPaint::Style style) override;
  void setStrokeWidth(SkScalar width) override;
  void setMiterLimit(SkScalar limit) override;
  void setImageFilter(sk_sp<SkImageFilter> filter) override;
  void setMaskFilter(sk_sp<SkMaskFilter> filter) override;
  void setMaskBlurFilter(SkBlurStyle style, SkScalar sigma) override;

  void saveLayer(const SkRect* bounds) override;
  void save() override;
  void restore() override;

  void drawPaint() override;
  void drawColor(SkColor color, SkBlendMode mode) override;
  void drawLine(const SkPoint& p0, const SkPoint& p1) override;
  void drawRect(const SkRect& rect) override;
  void drawOval(const SkRect& bounds) override;
  void drawCircle(const SkPoint& center, SkScalar radius) override;
  void drawRRect(const SkRRect& rrect) override;
  void drawDRRect(const SkRRect& outer, const SkRRect& inner) override;
  void drawPath(const SkPath& path) override;
  void drawArc(const SkRect& bounds,
               SkScalar start,
               SkScalar sweep,
               bool useCenter) override;
  void drawPoints(SkCanvas::PointMode mode,
                  size_t count,
                  const SkPoint pts[]) override;
  void drawVertices(const sk_sp<SkVertices> vertices,
                    SkBlendMode mode) override;
  void drawImage(const sk_sp<SkImage> image,
                 const SkPoint point,
                 const SkSamplingOptions& sampling) override;
  void drawImageRect(const sk_sp<SkImage> image,
                     const SkRect& src,
                     const SkRect& dst,
                     const SkSamplingOptions& sampling) override;
  void drawImageNine(const sk_sp<SkImage> image,
                     const SkRect& center,
                     const SkRect& dst,
                     SkFilterMode filter) override;
  void drawImageLattice(const sk_sp<SkImage> image,
                        const SkCanvas::Lattice& lattice,
                        const SkRect& dst,
                        SkFilterMode filter) override;
  void drawAtlas(const sk_sp<SkImage> atlas,
                 const SkRSXform xform[],
                 const SkRect tex[],
                 const SkColor colors[],
                 int count,
                 SkBlendMode mode,
                 const SkSamplingOptions& sampling,
                 const SkRect* cullRect) override;
  void drawPicture(const sk_sp<SkPicture> picture) override;
  void drawDisplayList(const sk_sp<DisplayList> display_list) override;
  void drawTextBlob(const sk_sp<SkTextBlob> blob,
                    SkScalar x,
                    SkScalar y) override;
  // void drawShadowRec(const SkPath&, const SkDrawShadowRec&) override;
  void drawShadow(const SkPath& path,
                  const SkColor color,
                  const SkScalar elevation,
                  bool occludes) override;

  SkRect getBounds() { return accumulator_.getBounds(); }

 private:
  enum BoundsType {
    GEOM_FILL,    // geometry, paintType is fill
    GEOM_STROKE,  // geometry, paintType is stroke or stroke+fill
    NON_GEOM      // i.e. image, picture, other non-geometric
  };

  bool isMiter_ = true;
  BoundsType geomType_;
  SkScalar strokeWidth_ = 1.0;
  SkScalar miterLimit_ = 4.0;
  sk_sp<SkMaskFilter> maskFilter_;
  SkBlurStyle maskBlurStyle_;
  SkScalar maskBlurSigma_ = 0.0;
  sk_sp<SkImageFilter> imageFilter_;

  BoundsAccumulator accumulator_;

  void accumulatePoint(const SkPoint& p0, BoundsType type);
  void accumulateRect(const SkRect& rect) { accumulateRect(rect, geomType_); }
  void accumulateRect(const SkRect& rect, BoundsType type);
};

}  // namespace flutter

#endif  // FLUTTER_FLOW_DISPLAY_LIST_UTILS_H_
