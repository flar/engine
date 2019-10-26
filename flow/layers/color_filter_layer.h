// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_FLOW_LAYERS_COLOR_FILTER_LAYER_H_
#define FLUTTER_FLOW_LAYERS_COLOR_FILTER_LAYER_H_

#include "flutter/flow/layers/container_layer.h"

#include "third_party/skia/include/core/SkColorFilter.h"

namespace flutter {

class ColorFilterLayer : public ContainerLayer {
 public:
  static std::shared_ptr<ColorFilterLayer> makeLayer(
      sk_sp<SkColorFilter> filter,
      std::shared_ptr<ColorFilterLayer> old_layer) {
    if (old_layer) {
      ColorFilterLayer* old_filter_layer = (ColorFilterLayer*)old_layer.get();
      if (old_filter_layer->filter_ == filter) {
        old_filter_layer->PrepareForNewChildren();
        return old_layer;
      }
    }
    return std::make_shared<flutter::ColorFilterLayer>(filter);
  }

  ColorFilterLayer(sk_sp<SkColorFilter> filter);
  ~ColorFilterLayer() override;

  void Paint(PaintContext& context) const override;

  std::string layer_type_name() const override { return "ColorFilterLayer"; }

 private:
  sk_sp<SkColorFilter> filter_;

  FML_DISALLOW_COPY_AND_ASSIGN(ColorFilterLayer);
};

}  // namespace flutter

#endif  // FLUTTER_FLOW_LAYERS_COLOR_FILTER_LAYER_H_
