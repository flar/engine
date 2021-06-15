// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_FLOW_LAYERS_DISPLAY_LIST_LAYER_H_
#define FLUTTER_FLOW_LAYERS_DISPLAY_LIST_LAYER_H_

#include "flutter/flow/display_list.h"
#include "flutter/flow/layers/layer.h"

namespace flutter {

class DisplayListLayer : public Layer {
 public:
  DisplayListLayer(const SkPoint& offset,
                   sk_sp<DisplayList> display_list,
                   bool is_complex,
                   bool will_change);

#ifdef FLUTTER_ENABLE_DIFF_CONTEXT

  void Diff(DiffContext* context, const Layer* old_layer) override;

#endif  // FLUTTER_ENABLE_DIFF_CONTEXT

  void Preroll(PrerollContext* frame, const SkMatrix& matrix) override;

  void Paint(PaintContext& context) const override;

 private:
  SkPoint offset_;
  sk_sp<DisplayList> display_list_;
  bool is_complex_ = false;
  bool will_change_ = false;

  FML_DISALLOW_COPY_AND_ASSIGN(DisplayListLayer);
};

}  // namespace flutter

#endif  // FLUTTER_FLOW_LAYERS_DISPLAY_LIST_LAYER_H_
