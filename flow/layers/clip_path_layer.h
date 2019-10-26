// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_FLOW_LAYERS_CLIP_PATH_LAYER_H_
#define FLUTTER_FLOW_LAYERS_CLIP_PATH_LAYER_H_

#include "flutter/flow/layers/container_layer.h"

namespace flutter {

class ClipPathLayer : public ContainerLayer {
 public:
  static std::shared_ptr<ClipPathLayer> makeLayer(
      const SkPath& clip_path,
      Clip clip_behavior,
      std::shared_ptr<ClipPathLayer> old_layer) {
    if (old_layer) {
      ClipPathLayer* old_clip_layer = (ClipPathLayer*)old_layer.get();
      if (old_clip_layer->clip_path_ == clip_path &&
          old_clip_layer->clip_behavior_ == clip_behavior) {
        old_clip_layer->PrepareForNewChildren();
        return old_layer;
      }
    }
    return std::make_shared<flutter::ClipPathLayer>(clip_path, clip_behavior);
  }

  ClipPathLayer(const SkPath& clip_path, Clip clip_behavior = Clip::antiAlias);
  ~ClipPathLayer() override;

  void Preroll(PrerollContext* context, const SkMatrix& matrix) override;

  void Paint(PaintContext& context) const override;

#if defined(OS_FUCHSIA)
  void UpdateScene(SceneUpdateContext& context) override;
#endif  // defined(OS_FUCHSIA)

  std::string layer_type_name() const override { return "ClipPathLayer"; }

 private:
  SkPath clip_path_;
  Clip clip_behavior_;

  FML_DISALLOW_COPY_AND_ASSIGN(ClipPathLayer);
};

}  // namespace flutter

#endif  // FLUTTER_FLOW_LAYERS_CLIP_PATH_LAYER_H_
