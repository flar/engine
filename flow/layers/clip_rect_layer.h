// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_FLOW_LAYERS_CLIP_RECT_LAYER_H_
#define FLUTTER_FLOW_LAYERS_CLIP_RECT_LAYER_H_

#include "flutter/flow/layers/container_layer.h"

namespace flutter {

class ClipRectLayer : public ContainerLayer {
 public:
  static std::shared_ptr<ClipRectLayer> makeLayer(
      const SkRect& clip_rect,
      Clip clip_behavior,
      std::shared_ptr<ClipRectLayer> old_layer) {
    if (old_layer) {
      ClipRectLayer* old_clip_layer = (ClipRectLayer*)old_layer.get();
      if (old_clip_layer->clip_rect_ == clip_rect &&
          old_clip_layer->clip_behavior_ == clip_behavior) {
        old_clip_layer->PrepareForNewChildren();
        return old_layer;
      }
    }
    return std::make_shared<flutter::ClipRectLayer>(clip_rect, clip_behavior);
  }

  ClipRectLayer(const SkRect& clip_rect, Clip clip_behavior);
  ~ClipRectLayer() override;

  void Preroll(PrerollContext* context, const SkMatrix& matrix) override;
  void Paint(PaintContext& context) const override;

#if defined(OS_FUCHSIA)
  void UpdateScene(SceneUpdateContext& context) override;
#endif  // defined(OS_FUCHSIA)

  std::string layer_type_name() const override { return "ClipRectLayer"; }

 private:
  SkRect clip_rect_;
  Clip clip_behavior_;

  FML_DISALLOW_COPY_AND_ASSIGN(ClipRectLayer);
};

}  // namespace flutter

#endif  // FLUTTER_FLOW_LAYERS_CLIP_RECT_LAYER_H_
