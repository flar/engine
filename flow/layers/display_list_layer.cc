// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/layers/display_list_layer.h"

#include "flutter/flow/display_list_canvas.h"

namespace flutter {

DisplayListLayer::DisplayListLayer(const SkPoint& offset,
                                   sk_sp<DisplayList> display_list,
                                   bool is_complex,
                                   bool will_change)
    : offset_(offset),
      display_list_(display_list),
      is_complex_(is_complex),
      will_change_(will_change) {}

#ifdef FLUTTER_ENABLE_DIFF_CONTEXT

// TODO(flar) Implement display list comparisons and ::IsReplacing method
void DisplayListLayer::Diff(DiffContext* context, const Layer* old_layer) {
  DiffContext::AutoSubtreeRestore subtree(context);
  auto* prev = static_cast<const DisplayListLayer*>(old_layer);
  if (!context->IsSubtreeDirty()) {
    FML_DCHECK(prev);
    if (offset_ != prev->offset_) {
      context->MarkSubtreeDirty(context->GetOldLayerPaintRegion(old_layer));
    }
  }
  context->PushTransform(SkMatrix::Translate(offset_.x(), offset_.y()));
  context->AddLayerBounds(display_list_->bounds());
  context->SetLayerPaintRegion(this, context->CurrentSubtreeRegion());
}

#endif  // FLUTTER_ENABLE_DIFF_CONTEXT

void DisplayListLayer::Preroll(PrerollContext* context,
                               const SkMatrix& matrix) {
  TRACE_EVENT0("flutter", "DisplayListLayer::Preroll");

#if defined(LEGACY_FUCHSIA_EMBEDDER)
  CheckForChildLayerBelow(context);
#endif

  // TODO(flar): implement DisplayList raster caching

  SkRect bounds = display_list_->bounds();
  bounds.offset(offset_.x(), offset_.y());
  set_paint_bounds(bounds);
}

void DisplayListLayer::Paint(PaintContext& context) const {
  TRACE_EVENT0("flutter", "DisplayListLayer::Paint");
  FML_DCHECK(needs_painting(context));

  SkAutoCanvasRestore save(context.leaf_nodes_canvas, true);
  context.leaf_nodes_canvas->translate(offset_.x(), offset_.y());
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
  context.leaf_nodes_canvas->setMatrix(RasterCache::GetIntegralTransCTM(
      context.leaf_nodes_canvas->getTotalMatrix()));
#endif

  // TODO(flar): implement DisplayList raster caching

  display_list_->renderTo(context.leaf_nodes_canvas);

  SkPaint paint;
  paint.setColor(is_complex_
                     ? (will_change_ ? SkColors::kRed : SkColors::kYellow)
                     : (will_change_ ? SkColors::kBlue : SkColors::kGreen));
}

}  // namespace flutter
