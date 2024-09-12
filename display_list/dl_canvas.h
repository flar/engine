// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_DISPLAY_LIST_DL_CANVAS_H_
#define FLUTTER_DISPLAY_LIST_DL_CANVAS_H_

#include "flutter/display_list/dl_blend_mode.h"
#include "flutter/display_list/dl_paint.h"
#include "flutter/display_list/dl_vertices.h"
#include "flutter/display_list/geometry/dl_geometry_types.h"
#include "flutter/display_list/geometry/dl_path.h"
#include "flutter/display_list/image/dl_image.h"

#include "third_party/skia/include/core/SkM44.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkRRect.h"
#include "third_party/skia/include/core/SkRSXform.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkTextBlob.h"

#include "impeller/typographer/text_frame.h"

namespace flutter {

//------------------------------------------------------------------------------
/// @brief    Developer-facing API for rendering anything *within* the engine.
///
/// |DlCanvas| should be used to render anything in the framework classes (i.e.
/// `lib/ui`), flow and flow layers, embedders, shell, and elsewhere.
///
/// The only state carried by implementations of this interface are the clip
/// and transform which are saved and restored by the |save|, |saveLayer|, and
/// |restore| calls.
///
/// @note      The interface resembles closely the familiar |SkCanvas| interface
///            used throughout the engine.
class DlCanvas {
 public:
  enum class ClipOp {
    kDifference,
    kIntersect,
  };

  enum class PointMode {
    kPoints,   //!< draw each point separately
    kLines,    //!< draw each separate pair of points as a line segment
    kPolygon,  //!< draw each pair of overlapping points as a line segment
  };

  enum class SrcRectConstraint {
    kStrict,
    kFast,
  };

  virtual ~DlCanvas() = default;

  virtual SkISize GetBaseLayerSize() const = 0;
  virtual SkImageInfo GetImageInfo() const = 0;

  virtual void Save() = 0;
  virtual void SaveLayer(const SkRect* bounds,
                         const DlPaint* paint = nullptr,
                         const DlImageFilter* backdrop = nullptr) = 0;
  virtual void Restore() = 0;
  virtual int GetSaveCount() const = 0;
  virtual void RestoreToCount(int restore_count) = 0;

  virtual void Translate(DlScalar tx, DlScalar ty) = 0;
  virtual void Scale(DlScalar sx, DlScalar sy) = 0;
  virtual void Rotate(DlScalar degrees) = 0;
  virtual void Skew(DlScalar sx, DlScalar sy) = 0;

  // clang-format off

  // 2x3 2D affine subset of a 4x4 transform in row major order
  virtual void Transform2DAffine(DlScalar mxx, DlScalar mxy, DlScalar mxt,
                                 DlScalar myx, DlScalar myy, DlScalar myt) = 0;
  // full 4x4 transform in row major order
  virtual void TransformFullPerspective(
      DlScalar mxx, DlScalar mxy, DlScalar mxz, DlScalar mxt,
      DlScalar myx, DlScalar myy, DlScalar myz, DlScalar myt,
      DlScalar mzx, DlScalar mzy, DlScalar mzz, DlScalar mzt,
      DlScalar mwx, DlScalar mwy, DlScalar mwz, DlScalar mwt) = 0;
  // clang-format on
  virtual void TransformReset() = 0;
  virtual void Transform(const SkMatrix* matrix) = 0;
  virtual void Transform(const SkM44* matrix44) = 0;
  void Transform(const SkMatrix& matrix) { Transform(&matrix); }
  void Transform(const SkM44& matrix44) { Transform(&matrix44); }
  virtual void SetTransform(const SkMatrix* matrix) = 0;
  virtual void SetTransform(const SkM44* matrix44) = 0;
  virtual void SetTransform(const SkMatrix& matrix) { SetTransform(&matrix); }
  virtual void SetTransform(const SkM44& matrix44) { SetTransform(&matrix44); }

  /// Returns the 4x4 full perspective transform representing all transform
  /// operations executed so far in this DisplayList within the enclosing
  /// save stack.
  virtual SkM44 GetTransformFullPerspective() const = 0;
  /// Returns the 3x3 partial perspective transform representing all transform
  /// operations executed so far in this DisplayList within the enclosing
  /// save stack.
  virtual SkMatrix GetTransform() const = 0;

  virtual void ClipRect(const SkRect& rect,
                        ClipOp clip_op = ClipOp::kIntersect,
                        bool is_aa = false) = 0;
  virtual void ClipOval(const SkRect& bounds,
                        ClipOp clip_op = ClipOp::kIntersect,
                        bool is_aa = false) = 0;
  virtual void ClipRRect(const SkRRect& rrect,
                         ClipOp clip_op = ClipOp::kIntersect,
                         bool is_aa = false) = 0;
  virtual void ClipPath(const SkPath& path,
                        ClipOp clip_op = ClipOp::kIntersect,
                        bool is_aa = false) = 0;
  virtual void ClipPath(const DlPath& path,
                        ClipOp clip_op = ClipOp::kIntersect,
                        bool is_aa = false) {
    ClipPath(path.GetSkPath(true), clip_op, is_aa);
  }

  /// Conservative estimate of the bounds of all outstanding clip operations
  /// measured in the coordinate space within which this DisplayList will
  /// be rendered.
  virtual SkRect GetDestinationClipBounds() const = 0;
  /// Conservative estimate of the bounds of all outstanding clip operations
  /// transformed into the local coordinate space in which currently
  /// recorded rendering operations are interpreted.
  virtual SkRect GetLocalClipBounds() const = 0;

  /// Return true iff the supplied bounds are easily shown to be outside
  /// of the current clip bounds. This method may conservatively return
  /// false if it cannot make the determination.
  virtual bool QuickReject(const SkRect& bounds) const = 0;

  virtual void DrawPaint(const DlPaint& paint) = 0;
  virtual void DrawColor(DlColor color,
                         DlBlendMode mode = DlBlendMode::kSrcOver) = 0;
  void Clear(DlColor color) { DrawColor(color, DlBlendMode::kSrc); }
  virtual void DrawLine(const SkPoint& p0,
                        const SkPoint& p1,
                        const DlPaint& paint) = 0;
  virtual void DrawDashedLine(const DlPoint& p0,
                              const DlPoint& p1,
                              DlScalar on_length,
                              DlScalar off_length,
                              const DlPaint& paint) = 0;
  virtual void DrawRect(const SkRect& rect, const DlPaint& paint) = 0;
  virtual void DrawOval(const SkRect& bounds, const DlPaint& paint) = 0;
  virtual void DrawCircle(const SkPoint& center,
                          DlScalar radius,
                          const DlPaint& paint) = 0;
  virtual void DrawRRect(const SkRRect& rrect, const DlPaint& paint) = 0;
  virtual void DrawDRRect(const SkRRect& outer,
                          const SkRRect& inner,
                          const DlPaint& paint) = 0;
  virtual void DrawPath(const SkPath& path, const DlPaint& paint) = 0;
  virtual void DrawPath(const DlPath& path, const DlPaint& paint) {
    DrawPath(path.GetSkPath(true), paint);
  }
  virtual void DrawArc(const SkRect& bounds,
                       DlScalar start,
                       DlScalar sweep,
                       bool useCenter,
                       const DlPaint& paint) = 0;
  virtual void DrawPoints(PointMode mode,
                          uint32_t count,
                          const SkPoint pts[],
                          const DlPaint& paint) = 0;
  virtual void DrawVertices(const std::shared_ptr<DlVertices>& vertices,
                            DlBlendMode mode,
                            const DlPaint& paint) = 0;
  virtual void DrawImage(const sk_sp<DlImage>& image,
                         const SkPoint& point,
                         DlImageSampling sampling,
                         const DlPaint* paint = nullptr) = 0;
  virtual void DrawImageRect(
      const sk_sp<DlImage>& image,
      const SkRect& src,
      const SkRect& dst,
      DlImageSampling sampling,
      const DlPaint* paint = nullptr,
      SrcRectConstraint constraint = SrcRectConstraint::kFast) = 0;
  virtual void DrawImageRect(
      const sk_sp<DlImage>& image,
      const SkIRect& src,
      const SkRect& dst,
      DlImageSampling sampling,
      const DlPaint* paint = nullptr,
      SrcRectConstraint constraint = SrcRectConstraint::kFast) {
    DrawImageRect(image, SkRect::Make(src), dst, sampling, paint, constraint);
  }
  void DrawImageRect(const sk_sp<DlImage>& image,
                     const SkRect& dst,
                     DlImageSampling sampling,
                     const DlPaint* paint = nullptr,
                     SrcRectConstraint constraint = SrcRectConstraint::kFast) {
    DrawImageRect(image, image->bounds(), dst, sampling, paint, constraint);
  }
  virtual void DrawImageNine(const sk_sp<DlImage>& image,
                             const SkIRect& center,
                             const SkRect& dst,
                             DlFilterMode filter,
                             const DlPaint* paint = nullptr) = 0;
  virtual void DrawAtlas(const sk_sp<DlImage>& atlas,
                         const SkRSXform xform[],
                         const SkRect tex[],
                         const DlColor colors[],
                         int count,
                         DlBlendMode mode,
                         DlImageSampling sampling,
                         const SkRect* cullRect,
                         const DlPaint* paint = nullptr) = 0;
  virtual void DrawDisplayList(const sk_sp<DisplayList> display_list,
                               DlScalar opacity = SK_Scalar1) = 0;

  virtual void DrawTextFrame(
      const std::shared_ptr<impeller::TextFrame>& text_frame,
      DlScalar x,
      DlScalar y,
      const DlPaint& paint) = 0;

  virtual void DrawTextBlob(const sk_sp<SkTextBlob>& blob,
                            DlScalar x,
                            DlScalar y,
                            const DlPaint& paint) = 0;
  virtual void DrawShadow(const SkPath& path,
                          const DlColor color,
                          const DlScalar elevation,
                          bool transparent_occluder,
                          DlScalar dpr) = 0;
  virtual void DrawShadow(const DlPath& path,
                          const DlColor color,
                          const DlScalar elevation,
                          bool transparent_occluder,
                          DlScalar dpr) {
    DrawShadow(path.GetSkPath(true), color, elevation, transparent_occluder,
               dpr);
  }

  virtual void Flush() = 0;

  static constexpr DlScalar kShadowLightHeight = 600;
  static constexpr DlScalar kShadowLightRadius = 800;

  static SkRect ComputeShadowBounds(const SkPath& path,
                                    float elevation,
                                    DlScalar dpr,
                                    const SkMatrix& ctm);
};

class DlAutoCanvasRestore {
 public:
  DlAutoCanvasRestore(DlCanvas* canvas, bool do_save) : canvas_(canvas) {
    if (canvas) {
      canvas_ = canvas;
      restore_count_ = canvas->GetSaveCount();
      if (do_save) {
        canvas_->Save();
      }
    } else {
      canvas_ = nullptr;
      restore_count_ = 0;
    }
  }

  ~DlAutoCanvasRestore() { Restore(); }

  void Restore() {
    if (canvas_) {
      canvas_->RestoreToCount(restore_count_);
      canvas_ = nullptr;
    }
  }

 private:
  DlCanvas* canvas_;
  int restore_count_;

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(DlAutoCanvasRestore);
};

}  // namespace flutter

#endif  // FLUTTER_DISPLAY_LIST_DL_CANVAS_H_
