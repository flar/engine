// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/layers/backdrop_filter_layer.h"

namespace flutter {

BackdropFilterLayer::BackdropFilterLayer(sk_sp<SkImageFilter> filter)
    : filter_(std::move(filter)) {}

BackdropFilterLayer::~BackdropFilterLayer() = default;

void BackdropFilterLayer::Preroll(PrerollContext* context,
                                  const SkMatrix& matrix) {
  if (!context->dirty_rect.isEmpty()) {
    FML_LOG(ERROR) << "******* BackdropFilterLayer must repaint due to dirty rectangle *******";
    set_painted(false);
  }
  ContainerLayer::Preroll(context, matrix);
  if (!is_painted()) {
    set_paint_bounds(kGiantRect);
  }
}

void BackdropFilterLayer::Paint(PaintContext& context) const {
  TRACE_EVENT0("flutter", "BackdropFilterLayer::Paint");
  FML_DCHECK(needs_painting());

  Layer::AutoSaveLayer save = Layer::AutoSaveLayer::Create(
      context,
      SkCanvas::SaveLayerRec{&paint_bounds(), nullptr, filter_.get(), 0});
  PaintChildren(context);
}

}  // namespace flutter
