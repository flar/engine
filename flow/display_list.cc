// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <type_traits>

#include "flutter/flow/display_list.h"
#include "flutter/flow/display_list_utils.h"
#include "flutter/fml/logging.h"

#include "third_party/skia/include/core/SkImageFilter.h"
#include "third_party/skia/include/core/SkMaskFilter.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkRSXform.h"
#include "third_party/skia/include/core/SkTextBlob.h"

// This header file cannot be included here, but we cannot
// record calls made by the SkShadowUtils without it.
// #include "third_party/skia/src/core/SkDrawShadowInfo.h"

namespace flutter {

const SkSamplingOptions DisplayList::NearestSampling =
    SkSamplingOptions(SkFilterMode::kNearest, SkMipmapMode::kNone);
const SkSamplingOptions DisplayList::LinearSampling =
    SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kNone);
const SkSamplingOptions DisplayList::MipmapSampling =
    SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kLinear);
const SkSamplingOptions DisplayList::CubicSampling =
    SkSamplingOptions(SkCubicResampler{1 / 3.0f, 1 / 3.0f});

struct DLOp {
  uint8_t type : 8;
  uint32_t size : 24;
};

#define DEFINE_SET_CLEAR_BOOL_OP(name)                        \
  struct Set##name##Op final : DLOp {                         \
    static const auto kType = DisplayListOpType::Set##name;   \
                                                              \
    Set##name##Op() {}                                        \
                                                              \
    void dispatch(Dispatcher& dispatcher) const {             \
      dispatcher.set##name(true);                             \
    }                                                         \
  };                                                          \
  struct Clear##name##Op final : DLOp {                       \
    static const auto kType = DisplayListOpType::Clear##name; \
                                                              \
    Clear##name##Op() {}                                      \
                                                              \
    void dispatch(Dispatcher& dispatcher) const {             \
      dispatcher.set##name(false);                            \
    }                                                         \
  };
DEFINE_SET_CLEAR_BOOL_OP(AA)
DEFINE_SET_CLEAR_BOOL_OP(Dither)
DEFINE_SET_CLEAR_BOOL_OP(InvertColors)
#undef DEFINE_SET_CLEAR_BOOL_OP

#define DEFINE_SET_ENUM_OP(ename, vname)                               \
  struct Set##ename##s##vname##Op final : DLOp {                       \
    static const auto kType = DisplayListOpType::Set##ename##s##vname; \
                                                                       \
    Set##ename##s##vname##Op() {}                                      \
                                                                       \
    void dispatch(Dispatcher& dispatcher) const {                      \
      dispatcher.set##ename(SkPaint::ename::k##vname##_##ename);       \
    }                                                                  \
  };
DEFINE_SET_ENUM_OP(Cap, Butt)
DEFINE_SET_ENUM_OP(Cap, Round)
DEFINE_SET_ENUM_OP(Cap, Square)
DEFINE_SET_ENUM_OP(Join, Bevel)
DEFINE_SET_ENUM_OP(Join, Miter)
DEFINE_SET_ENUM_OP(Join, Round)
#undef DEFINE_SET_ENUM_OP

struct SetFillStyleOp final : DLOp {
  static const auto kType = DisplayListOpType::SetFillStyle;

  SetFillStyleOp() {}

  void dispatch(Dispatcher& dispatcher) const {
    dispatcher.setDrawStyle(SkPaint::Style::kFill_Style);
  }
};
struct SetStrokeStyleOp final : DLOp {
  static const auto kType = DisplayListOpType::SetStrokeStyle;

  SetStrokeStyleOp() {}

  void dispatch(Dispatcher& dispatcher) const {
    dispatcher.setDrawStyle(SkPaint::Style::kStroke_Style);
  }
};
struct SetStrokeWidthOp final : DLOp {
  static const auto kType = DisplayListOpType::SetStrokeWidth;

  SetStrokeWidthOp(SkScalar width) : width(width) {}

  const SkScalar width;

  void dispatch(Dispatcher& dispatcher) const {
    dispatcher.setStrokeWidth(width);
  }
};
struct SetMiterLimitOp final : DLOp {
  static const auto kType = DisplayListOpType::SetMiterLimit;

  SetMiterLimitOp(SkScalar limit) : limit(limit) {}

  const SkScalar limit;

  void dispatch(Dispatcher& dispatcher) const {
    dispatcher.setMiterLimit(limit);
  }
};

struct SetColorOp final : DLOp {
  static const auto kType = DisplayListOpType::SetColor;

  SetColorOp(SkColor color) : color(color) {}

  const SkColor color;

  void dispatch(Dispatcher& dispatcher) const { dispatcher.setColor(color); }
};
struct SetBlendModeOp final : DLOp {
  static const auto kType = DisplayListOpType::SetBlendMode;

  SetBlendModeOp(SkBlendMode mode) : mode(mode) {}

  const SkBlendMode mode;

  void dispatch(Dispatcher& dispatcher) const { dispatcher.setBlendMode(mode); }
};

#define DEFINE_FQ_OP(name, quality)                                      \
  struct SetFilterQuality##name##Op final : DLOp {                       \
    static const auto kType = DisplayListOpType::SetFilterQuality##name; \
                                                                         \
    SetFilterQuality##name##Op() {}                                      \
                                                                         \
    void dispatch(Dispatcher& dispatcher) const {                        \
      dispatcher.setFilterQuality(quality);                              \
    }                                                                    \
  };
DEFINE_FQ_OP(Nearest, SkFilterQuality::kNone_SkFilterQuality)
DEFINE_FQ_OP(Linear, SkFilterQuality::kLow_SkFilterQuality)
DEFINE_FQ_OP(Mipmap, SkFilterQuality::kMedium_SkFilterQuality)
DEFINE_FQ_OP(Cubic, SkFilterQuality::kHigh_SkFilterQuality)
#undef DEFINE_FQ_OP

#define DEFINE_SET_CLEAR_SKREF_OP(name, field)                        \
  struct Clear##name##Op final : DLOp {                               \
    static const auto kType = DisplayListOpType::Clear##name;         \
                                                                      \
    Clear##name##Op() {}                                              \
                                                                      \
    void dispatch(Dispatcher& dispatcher) const {                     \
      dispatcher.set##name(nullptr);                                  \
    }                                                                 \
  };                                                                  \
  struct Set##name##Op final : DLOp {                                 \
    static const auto kType = DisplayListOpType::Set##name;           \
                                                                      \
    Set##name##Op(sk_sp<Sk##name> field) : field(std::move(field)) {} \
                                                                      \
    sk_sp<Sk##name> field;                                            \
                                                                      \
    void dispatch(Dispatcher& dispatcher) const {                     \
      dispatcher.set##name(field);                                    \
    }                                                                 \
  };
DEFINE_SET_CLEAR_SKREF_OP(Shader, shader)
DEFINE_SET_CLEAR_SKREF_OP(ImageFilter, filter)
DEFINE_SET_CLEAR_SKREF_OP(ColorFilter, filter)
DEFINE_SET_CLEAR_SKREF_OP(MaskFilter, filter)
#undef DEFINE_SET_CLEAR_SKREF_OP

#define DEFINE_MASK_BLUR_FILTER_OP(name, style)                           \
  struct SetMaskBlurFilter##name##Op final : DLOp {                       \
    static const auto kType = DisplayListOpType::SetMaskBlurFilter##name; \
                                                                          \
    SetMaskBlurFilter##name##Op(SkScalar sigma) : sigma(sigma) {}         \
                                                                          \
    SkScalar sigma;                                                       \
                                                                          \
    void dispatch(Dispatcher& dispatcher) const {                         \
      dispatcher.setMaskBlurFilter(style, sigma);                         \
    }                                                                     \
  };
DEFINE_MASK_BLUR_FILTER_OP(Normal, kNormal_SkBlurStyle)
DEFINE_MASK_BLUR_FILTER_OP(Solid, kSolid_SkBlurStyle)
DEFINE_MASK_BLUR_FILTER_OP(Inner, kInner_SkBlurStyle)
DEFINE_MASK_BLUR_FILTER_OP(Outer, kOuter_SkBlurStyle)
#undef DEFINE_MASK_BLUR_FILTER_OP

struct SaveOp final : DLOp {
  static const auto kType = DisplayListOpType::Save;

  SaveOp() {}

  void dispatch(Dispatcher& dispatcher) const { dispatcher.save(); }
};
struct SaveLayerOp final : DLOp {
  static const auto kType = DisplayListOpType::SaveLayer;

  SaveLayerOp() {}

  void dispatch(Dispatcher& dispatcher) const { dispatcher.saveLayer(nullptr); }
};
struct SaveLayerBoundsOp final : DLOp {
  static const auto kType = DisplayListOpType::SaveLayerBounds;

  SaveLayerBoundsOp(SkRect rect) : rect(rect) {}

  const SkRect rect;

  void dispatch(Dispatcher& dispatcher) const { dispatcher.saveLayer(&rect); }
};
struct RestoreOp final : DLOp {
  static const auto kType = DisplayListOpType::Restore;

  RestoreOp() {}

  void dispatch(Dispatcher& dispatcher) const { dispatcher.restore(); }
};

struct TranslateOp final : DLOp {
  static const auto kType = DisplayListOpType::Translate;

  TranslateOp(SkScalar tx, SkScalar ty) : tx(tx), ty(ty) {}

  const SkScalar tx;
  const SkScalar ty;

  void dispatch(Dispatcher& dispatcher) const { dispatcher.translate(tx, ty); }
};
struct ScaleOp final : DLOp {
  static const auto kType = DisplayListOpType::Scale;

  ScaleOp(SkScalar sx, SkScalar sy) : sx(sx), sy(sy) {}

  const SkScalar sx;
  const SkScalar sy;

  void dispatch(Dispatcher& dispatcher) const { dispatcher.scale(sx, sy); }
};
struct RotateOp final : DLOp {
  static const auto kType = DisplayListOpType::Rotate;

  RotateOp(SkScalar degrees) : degrees(degrees) {}

  const SkScalar degrees;

  void dispatch(Dispatcher& dispatcher) const { dispatcher.rotate(degrees); }
};
struct SkewOp final : DLOp {
  static const auto kType = DisplayListOpType::Skew;

  SkewOp(SkScalar sx, SkScalar sy) : sx(sx), sy(sy) {}

  const SkScalar sx;
  const SkScalar sy;

  void dispatch(Dispatcher& dispatcher) const { dispatcher.skew(sx, sy); }
};
struct Transform2x3Op final : DLOp {
  static const auto kType = DisplayListOpType::Transform2x3;

  Transform2x3Op(SkScalar mxx,
                 SkScalar mxy,
                 SkScalar mxt,
                 SkScalar myx,
                 SkScalar myy,
                 SkScalar myt)
      : mxx(mxx), mxy(mxy), mxt(mxt), myx(myx), myy(myy), myt(myt) {}

  const SkScalar mxx, mxy, mxt;
  const SkScalar myx, myy, myt;

  void dispatch(Dispatcher& dispatcher) const {
    dispatcher.transform2x3(mxx, mxy, mxt, myx, myy, myt);
  }
};
struct Transform3x3Op final : DLOp {
  static const auto kType = DisplayListOpType::Transform3x3;

  Transform3x3Op(SkScalar mxx,
                 SkScalar mxy,
                 SkScalar mxt,
                 SkScalar myx,
                 SkScalar myy,
                 SkScalar myt,
                 SkScalar px,
                 SkScalar py,
                 SkScalar pt)
      : mxx(mxx),
        mxy(mxy),
        mxt(mxt),
        myx(myx),
        myy(myy),
        myt(myt),
        px(px),
        py(py),
        pt(pt) {}

  const SkScalar mxx, mxy, mxt;
  const SkScalar myx, myy, myt;
  const SkScalar px, py, pt;

  void dispatch(Dispatcher& dispatcher) const {
    dispatcher.transform3x3(mxx, mxy, mxt, myx, myy, myt, px, py, pt);
  }
};

#define DEFINE_CLIP_RECT_OP(rtype, name_mod, isaa, clipop)              \
  struct Clip##rtype##name_mod##Op final : DLOp {                       \
    static const auto kType = DisplayListOpType::Clip##rtype##name_mod; \
                                                                        \
    Clip##rtype##name_mod##Op(Sk##rtype shape) : shape(shape) {}        \
                                                                        \
    const Sk##rtype shape;                                              \
                                                                        \
    void dispatch(Dispatcher& dispatcher) const {                       \
      dispatcher.clip##rtype(shape, isaa, SkClipOp::clipop);            \
    }                                                                   \
  };
DEFINE_CLIP_RECT_OP(Rect, , false, kIntersect)
DEFINE_CLIP_RECT_OP(Rect, AA, true, kIntersect)
DEFINE_CLIP_RECT_OP(Rect, Diff, false, kDifference)
DEFINE_CLIP_RECT_OP(Rect, AADiff, true, kDifference)
#undef DEFINE_CLIP_RECT_OP

#define DEFINE_CLIP_SHAPE_OP(rtype, name_mod, isaa)                     \
  struct Clip##rtype##name_mod##Op final : DLOp {                       \
    static const auto kType = DisplayListOpType::Clip##rtype##name_mod; \
                                                                        \
    Clip##rtype##name_mod##Op(Sk##rtype shape) : shape(shape) {}        \
                                                                        \
    const Sk##rtype shape;                                              \
                                                                        \
    void dispatch(Dispatcher& dispatcher) const {                       \
      dispatcher.clip##rtype(shape, isaa);                              \
    }                                                                   \
  };
DEFINE_CLIP_SHAPE_OP(RRect, , false)
DEFINE_CLIP_SHAPE_OP(RRect, AA, true)
DEFINE_CLIP_SHAPE_OP(Path, , false)
DEFINE_CLIP_SHAPE_OP(Path, AA, true)
#undef DEFINE_CLIP_SHAPE_OP

struct DrawPaintOp final : DLOp {
  static const auto kType = DisplayListOpType::DrawPaint;

  DrawPaintOp() {}

  void dispatch(Dispatcher& dispatcher) const { dispatcher.drawPaint(); }
};
struct DrawColorOp final : DLOp {
  static const auto kType = DisplayListOpType::DrawColor;

  DrawColorOp(SkColor color, SkBlendMode mode) : color(color), mode(mode) {}

  const SkColor color;
  const SkBlendMode mode;

  void dispatch(Dispatcher& dispatcher) const {
    dispatcher.drawColor(color, mode);
  }
};

#define DEFINE_DRAW_1ARG_OP(op_name, arg_type, arg_name)         \
  struct Draw##op_name##Op final : DLOp {                        \
    static const auto kType = DisplayListOpType::Draw##op_name;  \
                                                                 \
    Draw##op_name##Op(arg_type arg_name) : arg_name(arg_name) {} \
                                                                 \
    const arg_type arg_name;                                     \
                                                                 \
    void dispatch(Dispatcher& dispatcher) const {                \
      dispatcher.draw##op_name(arg_name);                        \
    }                                                            \
  };
DEFINE_DRAW_1ARG_OP(Rect, SkRect, rect)
DEFINE_DRAW_1ARG_OP(Oval, SkRect, oval)
DEFINE_DRAW_1ARG_OP(RRect, SkRRect, rrect)
DEFINE_DRAW_1ARG_OP(Path, SkPath, path)
#undef DEFINE_DRAW_1ARG_OP

#define DEFINE_DRAW_2ARG_OP(op_name, type1, name1, type2, name2) \
  struct Draw##op_name##Op final : DLOp {                        \
    static const auto kType = DisplayListOpType::Draw##op_name;  \
                                                                 \
    Draw##op_name##Op(type1 name1, type2 name2)                  \
        : name1(name1), name2(name2) {}                          \
                                                                 \
    const type1 name1;                                           \
    const type2 name2;                                           \
                                                                 \
    void dispatch(Dispatcher& dispatcher) const {                \
      dispatcher.draw##op_name(name1, name2);                    \
    }                                                            \
  };
DEFINE_DRAW_2ARG_OP(Line, SkPoint, p0, SkPoint, p1)
DEFINE_DRAW_2ARG_OP(Circle, SkPoint, center, SkScalar, radius)
DEFINE_DRAW_2ARG_OP(DRRect, SkRRect, outer, SkRRect, inner)
#undef DEFINE_DRAW_2ARG_OP

#define DEFINE_DRAW_ARC_OP(op_name, center)                          \
  struct Draw##op_name##Op final : DLOp {                            \
    static const auto kType = DisplayListOpType::Draw##op_name;      \
                                                                     \
    Draw##op_name##Op(SkRect bounds, SkScalar start, SkScalar sweep) \
        : bounds(bounds), start(start), sweep(sweep) {}              \
                                                                     \
    const SkRect bounds;                                             \
    const SkScalar start;                                            \
    const SkScalar sweep;                                            \
                                                                     \
    void dispatch(Dispatcher& dispatcher) const {                    \
      dispatcher.drawArc(bounds, start, sweep, center);              \
    }                                                                \
  };
DEFINE_DRAW_ARC_OP(Arc, false)
DEFINE_DRAW_ARC_OP(ArcCenter, true)
#undef DEFINE_DRAW_ARC_OP

#define DEFINE_DRAW_POINTS_OP(name, mode)                              \
  struct Draw##name##Op final : DLOp {                                 \
    static const auto kType = DisplayListOpType::Draw##name;           \
                                                                       \
    Draw##name##Op(size_t count) : count(count) {}                     \
                                                                       \
    const size_t count;                                                \
                                                                       \
    void dispatch(Dispatcher& dispatcher) const {                      \
      const SkPoint* pts = reinterpret_cast<const SkPoint*>(this + 1); \
      dispatcher.drawPoints(SkCanvas::PointMode::mode, count, pts);    \
    }                                                                  \
  };
DEFINE_DRAW_POINTS_OP(Points, kPoints_PointMode);
DEFINE_DRAW_POINTS_OP(Lines, kLines_PointMode);
DEFINE_DRAW_POINTS_OP(Polygon, kPolygon_PointMode);
#undef DEFINE_DRAW_POINTS_OP

struct DrawVerticesOp final : DLOp {
  static const auto kType = DisplayListOpType::DrawVertices;

  DrawVerticesOp(sk_sp<SkVertices> vertices, SkBlendMode mode)
      : vertices(std::move(vertices)), mode(mode) {}

  const sk_sp<SkVertices> vertices;
  const SkBlendMode mode;

  void dispatch(Dispatcher& dispatcher) const {
    dispatcher.drawVertices(vertices, mode);
  }
};

struct DrawImageOp final : DLOp {
  static const auto kType = DisplayListOpType::DrawImage;

  DrawImageOp(const sk_sp<SkImage> image,
              const SkPoint& point,
              const SkSamplingOptions& sampling)
      : image(std::move(image)), point(point), sampling(sampling) {}

  const sk_sp<SkImage> image;
  const SkPoint point;
  const SkSamplingOptions sampling;

  void dispatch(Dispatcher& dispatcher) const {
    dispatcher.drawImage(image, point, sampling);
  }
};

struct DrawImageRectOp final : DLOp {
  static const auto kType = DisplayListOpType::DrawImageRect;

  DrawImageRectOp(const sk_sp<SkImage> image,
                  const SkRect& src,
                  const SkRect& dst,
                  const SkSamplingOptions& sampling)
      : image(std::move(image)), src(src), dst(dst), sampling(sampling) {}

  const sk_sp<SkImage> image;
  const SkRect src;
  const SkRect dst;
  const SkSamplingOptions sampling;

  void dispatch(Dispatcher& dispatcher) const {
    dispatcher.drawImageRect(image, src, dst, sampling);
  }
};

struct DrawImageNineOp final : DLOp {
  static const auto kType = DisplayListOpType::DrawImageNine;

  DrawImageNineOp(const sk_sp<SkImage> image,
                  const SkIRect& center,
                  const SkRect& dst,
                  SkFilterMode filter)
      : image(std::move(image)), center(center), dst(dst), filter(filter) {}

  const sk_sp<SkImage> image;
  const SkIRect center;
  const SkRect dst;
  const SkFilterMode filter;

  void dispatch(Dispatcher& dispatcher) const {
    dispatcher.drawImageNine(image, center, dst, filter);
  }
};

struct DrawImageLatticeOp final : DLOp {
  static const auto kType = DisplayListOpType::DrawImageLattice;

  DrawImageLatticeOp(const sk_sp<SkImage> image,
                     int xDivCount,
                     int yDivCount,
                     int cellCount,
                     const SkIRect& src,
                     const SkRect& dst,
                     SkFilterMode filter)
      : image(std::move(image)),
        xDivCount(xDivCount),
        yDivCount(yDivCount),
        cellCount(cellCount),
        src(src),
        dst(dst),
        filter(filter) {}

  const sk_sp<SkImage> image;
  const int xDivCount;
  const int yDivCount;
  const int cellCount;
  const SkIRect src;
  const SkRect dst;
  const SkFilterMode filter;

  void dispatch(Dispatcher& dispatcher) const {
    const int* xDivs = reinterpret_cast<const int*>(this + 1);
    const int* yDivs = reinterpret_cast<const int*>(xDivs + xDivCount);
    const SkColor* colors =
        (cellCount == 0) ? nullptr
                         : reinterpret_cast<const SkColor*>(yDivs + yDivCount);
    const SkCanvas::Lattice::RectType* rTypes =
        (cellCount == 0) ? nullptr
                         : reinterpret_cast<const SkCanvas::Lattice::RectType*>(
                               colors + cellCount);
    dispatcher.drawImageLattice(
        image, {xDivs, yDivs, rTypes, xDivCount, yDivCount, &src, colors}, dst,
        filter);
  }
};

#define DRAW_ATLAS_NO_COLORS(tex, count) nullptr
#define DRAW_ATLAS_HAS_COLORS(tex, count) \
  reinterpret_cast<const SkColor*>(tex + count)

#define DRAW_ATLAS_NO_CULLING_ARGS                         \
  const sk_sp<SkImage> atlas, int count, SkBlendMode mode, \
      const SkSamplingOptions &sampling
#define DRAW_ATLAS_NO_CULLING_INIT \
  atlas(std::move(atlas)), count(count), mode(mode), sampling(sampling)
#define DRAW_ATLAS_NO_CULLING_FIELDS \
  const sk_sp<SkImage> atlas;        \
  const int count;                   \
  const SkBlendMode mode;            \
  const SkSamplingOptions sampling
#define DRAW_ATLAS_NO_CULLING_P_ARG nullptr

#define DRAW_ATLAS_HAS_CULLING_ARGS \
  DRAW_ATLAS_NO_CULLING_ARGS, const SkRect& cull
#define DRAW_ATLAS_HAS_CULLING_INIT DRAW_ATLAS_NO_CULLING_INIT, cull(cull)
#define DRAW_ATLAS_HAS_CULLING_FIELDS \
  DRAW_ATLAS_NO_CULLING_FIELDS;       \
  const SkRect cull
#define DRAW_ATLAS_HAS_CULLING_P_ARG &cull

#define DEFINE_DRAW_ATLAS_OP(name, colors, cull)                             \
  struct Draw##name##Op final : DLOp {                                       \
    static const auto kType = DisplayListOpType::Draw##name;                 \
                                                                             \
    Draw##name##Op(DRAW_ATLAS_##cull##_ARGS) : DRAW_ATLAS_##cull##_INIT {}   \
                                                                             \
    DRAW_ATLAS_##cull##_FIELDS;                                              \
                                                                             \
    void dispatch(Dispatcher& dispatcher) const {                            \
      const SkRSXform* xform = reinterpret_cast<const SkRSXform*>(this + 1); \
      const SkRect* tex = reinterpret_cast<const SkRect*>(xform + count);    \
      const SkColor* colors = DRAW_ATLAS_##colors(tex, count);               \
      dispatcher.drawAtlas(atlas, xform, tex, colors, count, mode, sampling, \
                           DRAW_ATLAS_##cull##_P_ARG);                       \
    }                                                                        \
  };
DEFINE_DRAW_ATLAS_OP(Atlas, NO_COLORS, NO_CULLING)
DEFINE_DRAW_ATLAS_OP(AtlasColored, HAS_COLORS, NO_CULLING)
DEFINE_DRAW_ATLAS_OP(AtlasCulled, NO_COLORS, HAS_CULLING)
DEFINE_DRAW_ATLAS_OP(AtlasColoredCulled, HAS_COLORS, HAS_CULLING)
#undef DEFINE_DRAW_ATLAS_OP
#undef DRAW_ATLAS_NO_COLORS
#undef DRAW_ATLAS_HAS_COLORS
#undef DRAW_ATLAS_NO_CULLING_ARGS
#undef DRAW_ATLAS_NO_CULLING_INIT
#undef DRAW_ATLAS_NO_CULLING_FIELDS
#undef DRAW_ATLAS_NO_CULLING_P_ARG
#undef DRAW_ATLAS_HAS_CULLING_ARGS
#undef DRAW_ATLAS_HAS_CULLING_INIT
#undef DRAW_ATLAS_HAS_CULLING_FIELDS
#undef DRAW_ATLAS_HAS_CULLING_P_ARG

struct DrawSkPictureOp final : DLOp {
  static const auto kType = DisplayListOpType::DrawSkPicture;

  DrawSkPictureOp(sk_sp<SkPicture> picture) : picture(std::move(picture)) {}

  sk_sp<SkPicture> picture;

  void dispatch(Dispatcher& dispatcher) const {
    dispatcher.drawPicture(picture);
  }
};

struct DrawDisplayListOp final : DLOp {
  static const auto kType = DisplayListOpType::DrawDisplayList;

  DrawDisplayListOp(const sk_sp<DisplayList> display_list)
      : display_list(std::move(display_list)) {}

  sk_sp<DisplayList> display_list;

  void dispatch(Dispatcher& dispatcher) const {
    dispatcher.drawDisplayList(display_list);
  }
};

struct DrawTextBlobOp final : DLOp {
  static const auto kType = DisplayListOpType::DrawTextBlob;

  DrawTextBlobOp(const sk_sp<SkTextBlob> blob, SkScalar x, SkScalar y)
      : blob(std::move(blob)), x(x), y(y) {}

  const sk_sp<SkTextBlob> blob;
  const SkScalar x;
  const SkScalar y;

  void dispatch(Dispatcher& dispatcher) const {
    dispatcher.drawTextBlob(blob, x, y);
  }
};

// struct DrawShadowRecOp final : DLOp {
//   static const auto kType = DisplayListOpType::DrawShadowRec;
//
//   DrawShadowRecOp(const SkPath& path, const SkDrawShadowRec& rec)
//       : path(path), rec(rec) {}
//
//   const SkPath path;
//   const SkDrawShadowRec rec;
//
//   void dispatch(Dispatcher& dispatcher) const {
//     dispatcher.drawShadowRec(path, rec);
//   }
// };

#define DEFINE_DRAW_SHADOW_OP(name, occludes)                             \
  struct Draw##name##Op final : DLOp {                                    \
    static const auto kType = DisplayListOpType::Draw##name;              \
                                                                          \
    Draw##name##Op(const SkPath& path, SkColor color, SkScalar elevation) \
        : path(path), color(color), elevation(elevation) {}               \
                                                                          \
    const SkPath path;                                                    \
    const SkColor color;                                                  \
    const SkScalar elevation;                                             \
                                                                          \
    void dispatch(Dispatcher& dispatcher) const {                         \
      dispatcher.drawShadow(path, color, elevation, occludes);            \
    }                                                                     \
  };
DEFINE_DRAW_SHADOW_OP(Shadow, false)
DEFINE_DRAW_SHADOW_OP(ShadowOccluded, true)

void DisplayList::computeBounds() {
  DisplayListBoundsCalculator calculator;
  dispatch(calculator);
  bounds_ = calculator.getBounds();
}

void DisplayList::dispatch(Dispatcher& dispatcher, uint8_t* ptr, uint8_t* end) {
  while (ptr < end) {
    auto op = (const DLOp*)ptr;
    ptr += op->size;
    FML_DCHECK(ptr <= end);
    switch (op->type) {
#define DL_OP_DISPATCH(name)                                \
  case DisplayListOpType::name:                             \
    static_cast<const name##Op*>(op)->dispatch(dispatcher); \
    break;

      FOR_EACH_DISPLAY_LIST_OP(DL_OP_DISPATCH)

#undef DL_OP_DISPATCH

      default:
        FML_DCHECK(false);
        return;
    }
  }
}

static void DisposeOps(uint8_t* ptr, uint8_t* end) {
  while (ptr < end) {
    auto op = (const DLOp*)ptr;
    ptr += op->size;
    FML_DCHECK(ptr <= end);
    switch (op->type) {
#define DL_OP_DISPOSE(name)                            \
  case DisplayListOpType::name:                        \
    if (!std::is_trivially_destructible_v<name##Op>) { \
      static_cast<const name##Op*>(op)->~name##Op();   \
    }                                                  \
    break;

      FOR_EACH_DISPLAY_LIST_OP(DL_OP_DISPOSE)

#undef DL_OP_DISPATCH

      default:
        FML_DCHECK(false);
        return;
    }
  }
}

DisplayList::~DisplayList() {
  DisposeOps(ptr_, ptr_ + used_);
}

#define DL_BUILDER_PAGE 4096

// copy_v(dst, src,n, src,n, ...) copies any number of typed srcs into dst.
static void copy_v(void* dst) {}

template <typename S, typename... Rest>
static void copy_v(void* dst, const S* src, int n, Rest&&... rest) {
  SkASSERTF(((uintptr_t)dst & (alignof(S) - 1)) == 0,
            "Expected %p to be aligned for at least %zu bytes.", dst,
            alignof(S));
  sk_careful_memcpy(dst, src, n * sizeof(S));
  copy_v(SkTAddOffset<void>(dst, n * sizeof(S)), std::forward<Rest>(rest)...);
}

template <typename T, typename... Args>
void* DisplayListBuilder::push(size_t pod, Args&&... args) {
  size_t size = SkAlignPtr(sizeof(T) + pod);
  SkASSERT(size < (1 << 24));
  if (used_ + size > allocated_) {
    static_assert(SkIsPow2(DL_BUILDER_PAGE),
                  "This math needs updating for non-pow2.");
    // Next greater multiple of DL_BUILDER_PAGE.
    allocated_ = (used_ + size + DL_BUILDER_PAGE) & ~(DL_BUILDER_PAGE - 1);
    storage_.realloc(allocated_);
    FML_DCHECK(storage_.get());
  }
  SkASSERT(used_ + size <= allocated_);
  auto op = (T*)(storage_.get() + used_);
  used_ += size;
  new (op) T{std::forward<Args>(args)...};
  op->type = (uint32_t)T::kType;
  op->size = size;
  return op + 1;
}

sk_sp<DisplayList> DisplayListBuilder::build() {
  while (saveLevel_ > 0) {
    restore();
  }
  size_t used = used_;
  used_ = allocated_ = 0;
  storage_.realloc(used);
  return sk_sp<DisplayList>(new DisplayList(storage_.release(), used));
}

DisplayListBuilder::~DisplayListBuilder() {
  uint8_t* ptr = storage_.get();
  if (ptr) {
    DisposeOps(ptr, ptr + used_);
  }
}

void DisplayListBuilder::setAA(bool aa) {
  aa  //
      ? push<SetAAOp>(0)
      : push<ClearAAOp>(0);
}
void DisplayListBuilder::setDither(bool dither) {
  dither  //
      ? push<SetDitherOp>(0)
      : push<ClearDitherOp>(0);
}
void DisplayListBuilder::setInvertColors(bool invert) {
  invert  //
      ? push<SetInvertColorsOp>(0)
      : push<ClearInvertColorsOp>(0);
}
void DisplayListBuilder::setCap(SkPaint::Cap cap) {
  switch (cap) {
    case SkPaint::Cap::kButt_Cap:
      push<SetCapsButtOp>(0);
      break;
    case SkPaint::Cap::kRound_Cap:
      push<SetCapsRoundOp>(0);
      break;
    case SkPaint::Cap::kSquare_Cap:
      push<SetCapsSquareOp>(0);
      break;
    default:
      FML_DCHECK(false);
  }
}
void DisplayListBuilder::setJoin(SkPaint::Join join) {
  switch (join) {
    case SkPaint::Join::kMiter_Join:
      push<SetJoinsMiterOp>(0);
      break;
    case SkPaint::Join::kRound_Join:
      push<SetJoinsRoundOp>(0);
      break;
    case SkPaint::Join::kBevel_Join:
      push<SetJoinsBevelOp>(0);
      break;
    default:
      FML_DCHECK(false);
  }
}
void DisplayListBuilder::setDrawStyle(SkPaint::Style style) {
  switch (style) {
    case SkPaint::Style::kFill_Style:
      push<SetFillStyleOp>(0);
      break;
    case SkPaint::Style::kStroke_Style:
      push<SetStrokeStyleOp>(0);
      break;
    default:
      FML_DCHECK(false);
  }
}
void DisplayListBuilder::setStrokeWidth(SkScalar width) {
  push<SetStrokeWidthOp>(0, width);
}
void DisplayListBuilder::setMiterLimit(SkScalar limit) {
  push<SetMiterLimitOp>(0, limit);
}
void DisplayListBuilder::setColor(SkColor color) {
  push<SetColorOp>(0, color);
}
void DisplayListBuilder::setBlendMode(SkBlendMode mode) {
  push<SetBlendModeOp>(0, mode);
}
void DisplayListBuilder::setFilterQuality(SkFilterQuality quality) {
  switch (quality) {
    case SkFilterQuality::kNone_SkFilterQuality:
      push<SetFilterQualityNearestOp>(0);
      break;
    case SkFilterQuality::kLow_SkFilterQuality:
      push<SetFilterQualityLinearOp>(0);
      break;
    case SkFilterQuality::kMedium_SkFilterQuality:
      push<SetFilterQualityMipmapOp>(0);
      break;
    case SkFilterQuality::kHigh_SkFilterQuality:
      push<SetFilterQualityCubicOp>(0);
      break;
  }
}
void DisplayListBuilder::setShader(sk_sp<SkShader> shader) {
  shader  //
      ? push<SetShaderOp>(0, std::move(shader))
      : push<ClearShaderOp>(0);
}
void DisplayListBuilder::setImageFilter(sk_sp<SkImageFilter> filter) {
  filter  //
      ? push<SetImageFilterOp>(0, std::move(filter))
      : push<ClearImageFilterOp>(0);
}
void DisplayListBuilder::setColorFilter(sk_sp<SkColorFilter> filter) {
  filter  //
      ? push<SetColorFilterOp>(0, std::move(filter))
      : push<ClearColorFilterOp>(0);
}
void DisplayListBuilder::setMaskFilter(sk_sp<SkMaskFilter> filter) {
  push<SetMaskFilterOp>(0, std::move(filter));
}
void DisplayListBuilder::setMaskBlurFilter(SkBlurStyle style, SkScalar sigma) {
  switch (style) {
    case kNormal_SkBlurStyle:
      push<SetMaskBlurFilterNormalOp>(0, sigma);
      break;
    case kSolid_SkBlurStyle:
      push<SetMaskBlurFilterSolidOp>(0, sigma);
      break;
    case kOuter_SkBlurStyle:
      push<SetMaskBlurFilterOuterOp>(0, sigma);
      break;
    case kInner_SkBlurStyle:
      push<SetMaskBlurFilterInnerOp>(0, sigma);
      break;
  }
}

void DisplayListBuilder::save() {
  saveLevel_++;
  push<SaveOp>(0);
}
void DisplayListBuilder::restore() {
  if (saveLevel_ > 0) {
    push<RestoreOp>(0);
    saveLevel_--;
  }
}
void DisplayListBuilder::saveLayer(const SkRect* bounds) {
  saveLevel_++;
  bounds  //
      ? push<SaveLayerBoundsOp>(0, *bounds)
      : push<SaveLayerOp>(0);
}

void DisplayListBuilder::translate(SkScalar tx, SkScalar ty) {
  push<TranslateOp>(0, tx, ty);
}
void DisplayListBuilder::scale(SkScalar sx, SkScalar sy) {
  push<ScaleOp>(0, sx, sy);
}
void DisplayListBuilder::rotate(SkScalar degrees) {
  push<RotateOp>(0, degrees);
}
void DisplayListBuilder::skew(SkScalar sx, SkScalar sy) {
  push<SkewOp>(0, sx, sy);
}
void DisplayListBuilder::transform2x3(SkScalar mxx,
                                      SkScalar mxy,
                                      SkScalar mxt,
                                      SkScalar myx,
                                      SkScalar myy,
                                      SkScalar myt) {
  push<Transform2x3Op>(0, mxx, mxy, mxt, myx, myy, myt);
}
void DisplayListBuilder::transform3x3(SkScalar mxx,
                                      SkScalar mxy,
                                      SkScalar mxt,
                                      SkScalar myx,
                                      SkScalar myy,
                                      SkScalar myt,
                                      SkScalar px,
                                      SkScalar py,
                                      SkScalar pt) {
  push<Transform3x3Op>(0, mxx, mxy, mxt, myx, myy, myt, px, py, pt);
}

void DisplayListBuilder::clipRect(const SkRect& rect,
                                  bool isAA,
                                  SkClipOp clip_op) {
  if (isAA) {
    if (clip_op == SkClipOp::kIntersect) {
      push<ClipRectOp>(0, rect);
    } else {
      push<ClipRectDiffOp>(0, rect);
    }
  } else {
    if (clip_op == SkClipOp::kIntersect) {
      push<ClipRectAAOp>(0, rect);
    } else {
      push<ClipRectAADiffOp>(0, rect);
    }
  }
}
void DisplayListBuilder::clipRRect(const SkRRect& rrect, bool isAA) {
  isAA  //
      ? push<ClipRRectOp>(0, rrect)
      : push<ClipRRectAAOp>(0, rrect);
}
void DisplayListBuilder::clipPath(const SkPath& path, bool isAA) {
  isAA  //
      ? push<ClipPathOp>(0, path)
      : push<ClipPathAAOp>(0, path);
}

void DisplayListBuilder::drawPaint() {
  push<DrawPaintOp>(0);
}
void DisplayListBuilder::drawColor(SkColor color, SkBlendMode mode) {
  push<DrawColorOp>(0, color, mode);
}
void DisplayListBuilder::drawLine(const SkPoint& p0, const SkPoint& p1) {
  push<DrawLineOp>(0, p0, p1);
}
void DisplayListBuilder::drawRect(const SkRect& rect) {
  push<DrawRectOp>(0, rect);
}
void DisplayListBuilder::drawOval(const SkRect& bounds) {
  push<DrawOvalOp>(0, bounds);
}
void DisplayListBuilder::drawCircle(const SkPoint& center, SkScalar radius) {
  push<DrawCircleOp>(0, center, radius);
}
void DisplayListBuilder::drawRRect(const SkRRect& rrect) {
  if (rrect.isRect()) {
    drawRect(rrect.rect());
  } else if (rrect.isOval()) {
    drawOval(rrect.rect());
  } else {
    push<DrawRRectOp>(0, rrect);
  }
}
void DisplayListBuilder::drawDRRect(const SkRRect& outer,
                                    const SkRRect& inner) {
  push<DrawDRRectOp>(0, outer, inner);
}
void DisplayListBuilder::drawPath(const SkPath& path) {
  push<DrawPathOp>(0, path);
}

void DisplayListBuilder::drawArc(const SkRect& bounds,
                                 SkScalar start,
                                 SkScalar sweep,
                                 bool useCenter) {
  if (useCenter) {
    push<DrawArcCenterOp>(0, bounds, start, sweep);
  } else {
    push<DrawArcOp>(0, bounds, start, sweep);
  }
}
void DisplayListBuilder::drawPoints(SkCanvas::PointMode mode,
                                    size_t count,
                                    const SkPoint pts[]) {
  void* data_ptr;
  int bytes = count * sizeof(SkPoint);
  switch (mode) {
    case SkCanvas::PointMode::kPoints_PointMode:
      data_ptr = push<DrawPointsOp>(bytes, count);
      break;
    case SkCanvas::PointMode::kLines_PointMode:
      data_ptr = push<DrawLinesOp>(bytes, count);
      break;
    case SkCanvas::PointMode::kPolygon_PointMode:
      data_ptr = push<DrawPolygonOp>(bytes, count);
      break;
    default:
      FML_DCHECK(false);
      return;
  }
  copy_v(data_ptr, pts, count);
}
void DisplayListBuilder::drawVertices(const sk_sp<SkVertices> vertices,
                                      SkBlendMode mode) {
  push<DrawVerticesOp>(0, std::move(vertices), mode);
}

void DisplayListBuilder::drawImage(const sk_sp<SkImage> image,
                                   const SkPoint point,
                                   const SkSamplingOptions& sampling) {
  push<DrawImageOp>(0, std::move(image), point, sampling);
}
void DisplayListBuilder::drawImageRect(const sk_sp<SkImage> image,
                                       const SkRect& src,
                                       const SkRect& dst,
                                       const SkSamplingOptions& sampling) {
  push<DrawImageRectOp>(0, std::move(image), src, dst, sampling);
}
void DisplayListBuilder::drawImageNine(const sk_sp<SkImage> image,
                                       const SkIRect& center,
                                       const SkRect& dst,
                                       SkFilterMode filter) {
  push<DrawImageNineOp>(0, std::move(image), center, dst, filter);
}
void DisplayListBuilder::drawImageLattice(const sk_sp<SkImage> image,
                                          const SkCanvas::Lattice& lattice,
                                          const SkRect& dst,
                                          SkFilterMode filter) {
  int xDivCount = lattice.fXCount;
  int yDivCount = lattice.fYCount;
  int cellCount = lattice.fRectTypes ? (xDivCount + 1) * (yDivCount + 1) : 0;
  size_t bytes =
      (xDivCount + yDivCount) * sizeof(int) +
      cellCount * (sizeof(SkColor) + sizeof(SkCanvas::Lattice::RectType));
  SkASSERT(lattice.fBounds);
  void* pod = this->push<DrawImageLatticeOp>(bytes, std::move(image), xDivCount,
                                             yDivCount, cellCount,
                                             *lattice.fBounds, dst, filter);
  copy_v(pod, lattice.fXDivs, xDivCount, lattice.fYDivs, yDivCount,
         lattice.fColors, cellCount, lattice.fRectTypes, cellCount);
}
void DisplayListBuilder::drawAtlas(const sk_sp<SkImage> atlas,
                                   const SkRSXform xform[],
                                   const SkRect tex[],
                                   const SkColor colors[],
                                   int count,
                                   SkBlendMode mode,
                                   const SkSamplingOptions& sampling,
                                   const SkRect* cullRect) {
  int bytes = count * (sizeof(SkRSXform) + sizeof(SkRect));
  void* data_ptr;
  if (colors) {
    bytes += count * sizeof(SkColor);
    if (cullRect) {
      data_ptr = push<DrawAtlasColoredCulledOp>(bytes, std::move(atlas), count,
                                                mode, sampling, *cullRect);
    } else {
      data_ptr = push<DrawAtlasColoredOp>(bytes, std::move(atlas), count, mode,
                                          sampling);
    }
    copy_v(data_ptr, xform, count, tex, count, colors, count);
  } else {
    if (cullRect) {
      data_ptr = push<DrawAtlasCulledOp>(bytes, std::move(atlas), count, mode,
                                         sampling, *cullRect);
    } else {
      data_ptr =
          push<DrawAtlasOp>(bytes, std::move(atlas), count, mode, sampling);
    }
    copy_v(data_ptr, xform, count, tex, count);
  }
}

void DisplayListBuilder::drawPicture(const sk_sp<SkPicture> picture) {
  push<DrawSkPictureOp>(0, std::move(picture));
}
void DisplayListBuilder::drawDisplayList(
    const sk_sp<DisplayList> display_list) {
  push<DrawDisplayListOp>(0, std::move(display_list));
}
void DisplayListBuilder::drawTextBlob(const sk_sp<SkTextBlob> blob,
                                      SkScalar x,
                                      SkScalar y) {
  push<DrawTextBlobOp>(0, std::move(blob), x, y);
}
// void DisplayListBuilder::drawShadowRec(const SkPath& path,
//                                        const SkDrawShadowRec& rec) {
//   push<DrawShadowRecOp>(0, path, rec);
// }
void DisplayListBuilder::drawShadow(const SkPath& path,
                                    const SkColor color,
                                    const SkScalar elevation,
                                    bool occludes) {
  if (occludes) {
    push<DrawShadowOccludedOp>(0, path, color, elevation);
  } else {
    push<DrawShadowOp>(0, path, color, elevation);
  }
}

}  // namespace flutter
