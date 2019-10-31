// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_FLOW_LAYERS_TRANSFORM_LAYER_H_
#define FLUTTER_FLOW_LAYERS_TRANSFORM_LAYER_H_

#include "flutter/flow/layers/container_layer.h"

namespace flutter {

// Be careful that SkMatrix's default constructor doesn't initialize the matrix
// at all. Hence |set_transform| must be called with an initialized SkMatrix.
class TransformLayer : public ContainerLayer {
 public:
  static std::shared_ptr<TransformLayer> makeLayer(
      const SkMatrix& transform,
      std::shared_ptr<TransformLayer> old_layer) {
    if (old_layer) {
      TransformLayer* old_tx_layer = (TransformLayer*)old_layer.get();
      if (old_tx_layer->transform_ == transform) {
        old_tx_layer->PrepareForNewChildren();
        return old_layer;
      }
    }
    return std::make_shared<flutter::TransformLayer>(transform);
  }

  TransformLayer(const SkMatrix& transform);
  ~TransformLayer() override;

  void Preroll(PrerollContext* context, const SkMatrix& matrix) override;

  void Paint(PaintContext& context) const override;

  std::string layer_type_name() const override { return "TransformLayer"; }

#if defined(OS_FUCHSIA)
  void UpdateScene(SceneUpdateContext& context) override;
#endif  // defined(OS_FUCHSIA)

 private:
  SkMatrix transform_;

  FML_DISALLOW_COPY_AND_ASSIGN(TransformLayer);
};

}  // namespace flutter

#endif  // FLUTTER_FLOW_LAYERS_TRANSFORM_LAYER_H_
