// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_FLOW_LAYERS_BACKDROP_FILTER_LAYER_H_
#define FLUTTER_FLOW_LAYERS_BACKDROP_FILTER_LAYER_H_

#include "flutter/flow/layers/container_layer.h"

#include "third_party/skia/include/core/SkImageFilter.h"

namespace flutter {

class BackdropFilterLayer : public ContainerLayer {
 public:
  static std::shared_ptr<BackdropFilterLayer> makeLayer(
      sk_sp<SkImageFilter> filter,
      std::shared_ptr<BackdropFilterLayer> old_layer) {
    if (old_layer) {
      BackdropFilterLayer* old_filter_layer =
          (BackdropFilterLayer*)old_layer.get();
      if (old_filter_layer->filter_ == filter) {
        old_filter_layer->PrepareForNewChildren();
        return old_layer;
      }
      FML_LOG(ERROR) << "Can't reuse BDF filter " << old_filter_layer->filter_ << " != " << filter;
    }
    return std::make_shared<flutter::BackdropFilterLayer>(filter);
  }

  BackdropFilterLayer(sk_sp<SkImageFilter> filter);
  ~BackdropFilterLayer() override;

  void Preroll(PrerollContext* context, const SkMatrix& matrix) override;

  void Paint(PaintContext& context) const override;

  std::string layer_type_name() const override { return "BackdropFilterLayer"; }

 private:
  sk_sp<SkImageFilter> filter_;

  FML_DISALLOW_COPY_AND_ASSIGN(BackdropFilterLayer);
};

}  // namespace flutter

#endif  // FLUTTER_FLOW_LAYERS_BACKDROP_FILTER_LAYER_H_
