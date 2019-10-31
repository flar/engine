// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_FLOW_LAYERS_PHYSICAL_SHAPE_LAYER_H_
#define FLUTTER_FLOW_LAYERS_PHYSICAL_SHAPE_LAYER_H_

#include "flutter/flow/layers/container_layer.h"

namespace flutter {

class PhysicalShapeLayer : public ContainerLayer {
 public:
  static std::shared_ptr<PhysicalShapeLayer> makeLayer(
      SkColor color,
      SkColor shadow_color,
      SkScalar device_pixel_ratio,
      float viewport_depth,
      float elevation,
      const SkPath& path,
      Clip clip_behavior,
      std::shared_ptr<PhysicalShapeLayer> old_layer) {
    if (old_layer) {
      PhysicalShapeLayer* old_shape_layer =
          (PhysicalShapeLayer*)old_layer.get();
      if (old_shape_layer->color_ == color &&
          old_shape_layer->shadow_color_ == shadow_color &&
          old_shape_layer->device_pixel_ratio_ == device_pixel_ratio &&
          old_shape_layer->viewport_depth_ == viewport_depth &&
          old_shape_layer->elevation_ == elevation &&
          old_shape_layer->path_ == path &&
          old_shape_layer->clip_behavior_ == clip_behavior) {
        old_shape_layer->PrepareForNewChildren();
        return old_layer;
      }
    }
    return std::make_shared<flutter::PhysicalShapeLayer>(
        color, shadow_color, device_pixel_ratio, viewport_depth, elevation,
        path, clip_behavior);
  }

  PhysicalShapeLayer(SkColor color,
                     SkColor shadow_color,
                     SkScalar device_pixel_ratio,
                     float viewport_depth,
                     float elevation,
                     const SkPath& path,
                     Clip clip_behavior);
  ~PhysicalShapeLayer() override;

  static void DrawShadow(SkCanvas* canvas,
                         const SkPath& path,
                         SkColor color,
                         float elevation,
                         bool transparentOccluder,
                         SkScalar dpr);

  void Preroll(PrerollContext* context, const SkMatrix& matrix) override;

  void Paint(PaintContext& context) const override;

#if defined(OS_FUCHSIA)
  void UpdateScene(SceneUpdateContext& context) override;
#endif  // defined(OS_FUCHSIA)

  std::string layer_type_name() const override { return "PhysicalShapeLayer"; }

 private:
  SkColor color_;
  SkColor shadow_color_;
  SkScalar device_pixel_ratio_;
  float viewport_depth_;
  float elevation_ = 0.0f;
  float total_elevation_ = 0.0f;
  SkPath path_;
  bool isRect_;
  SkRRect frameRRect_;
  Clip clip_behavior_;

  friend class PhysicalShapeLayer_TotalElevation_Test;
};

}  // namespace flutter

#endif  // FLUTTER_FLOW_LAYERS_PHYSICAL_SHAPE_LAYER_H_
