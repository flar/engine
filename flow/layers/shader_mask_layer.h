// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_FLOW_LAYERS_SHADER_MASK_LAYER_H_
#define FLUTTER_FLOW_LAYERS_SHADER_MASK_LAYER_H_

#include "flutter/flow/layers/container_layer.h"

#include "third_party/skia/include/core/SkShader.h"

namespace flutter {

class ShaderMaskLayer : public ContainerLayer {
 public:
  static std::shared_ptr<ShaderMaskLayer> makeLayer(
      sk_sp<SkShader> shader,
      const SkRect& mask_rect,
      SkBlendMode blend_mode,
      std::shared_ptr<ShaderMaskLayer> old_layer) {
    if (old_layer) {
      ShaderMaskLayer* old_filter_layer = (ShaderMaskLayer*)old_layer.get();
      if (old_filter_layer->shader_ == shader &&
          old_filter_layer->mask_rect_ == mask_rect &&
          old_filter_layer->blend_mode_ == blend_mode) {
        old_filter_layer->PrepareForNewChildren();
        return old_layer;
      }
    }
    return std::make_shared<flutter::ShaderMaskLayer>(shader, mask_rect,
                                                      blend_mode);
  }

  ShaderMaskLayer(sk_sp<SkShader> shader,
                  const SkRect& mask_rect,
                  SkBlendMode blend_mode);
  ~ShaderMaskLayer() override;

  void Paint(PaintContext& context) const override;

  std::string layer_type_name() const override { return "ShaderMaskLayer"; }

 private:
  sk_sp<SkShader> shader_;
  SkRect mask_rect_;
  SkBlendMode blend_mode_;

  FML_DISALLOW_COPY_AND_ASSIGN(ShaderMaskLayer);
};

}  // namespace flutter

#endif  // FLUTTER_FLOW_LAYERS_SHADER_MASK_LAYER_H_
