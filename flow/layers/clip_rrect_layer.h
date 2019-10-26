// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_FLOW_LAYERS_CLIP_RRECT_LAYER_H_
#define FLUTTER_FLOW_LAYERS_CLIP_RRECT_LAYER_H_

#include "flutter/flow/layers/container_layer.h"

namespace flutter {

class ClipRRectLayer : public ContainerLayer {
 public:
  static std::shared_ptr<ClipRRectLayer> makeLayer(
      const SkRRect& clip_rrect,
      Clip clip_behavior,
      std::shared_ptr<ClipRRectLayer> old_layer) {
    if (old_layer) {
      ClipRRectLayer* old_clip_layer = (ClipRRectLayer*)old_layer.get();
      if (old_clip_layer->clip_rrect_ == clip_rrect &&
          old_clip_layer->clip_behavior_ == clip_behavior) {
        old_clip_layer->PrepareForNewChildren();
        return old_layer;
      }
    }
    return std::make_shared<flutter::ClipRRectLayer>(clip_rrect, clip_behavior);
  }

  ClipRRectLayer(const SkRRect& clip_rrect, Clip clip_behavior);
  ~ClipRRectLayer() override;

  void Preroll(PrerollContext* context, const SkMatrix& matrix) override;

  void Paint(PaintContext& context) const override;

#if defined(OS_FUCHSIA)
  void UpdateScene(SceneUpdateContext& context) override;
#endif  // defined(OS_FUCHSIA)

  std::string layer_type_name() const override { return "ClipRRectLayer"; }

 private:
  SkRRect clip_rrect_;
  Clip clip_behavior_;

  FML_DISALLOW_COPY_AND_ASSIGN(ClipRRectLayer);
};

}  // namespace flutter

#endif  // FLUTTER_FLOW_LAYERS_CLIP_RRECT_LAYER_H_
