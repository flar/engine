// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/layers/opacity_layer.h"

#include "flutter/fml/trace_event.h"
#include "third_party/skia/include/core/SkPaint.h"

namespace flutter {

OpacityLayer::OpacityLayer(SkAlpha alpha, const SkPoint& offset)
    : alpha_(alpha), offset_(offset) {}

void OpacityLayer::Diff(DiffContext* context, const Layer* old_layer) {
  DiffContext::AutoSubtreeRestore subtree(context);
  auto* prev = static_cast<const OpacityLayer*>(old_layer);
  if (!context->IsSubtreeDirty()) {
    FML_DCHECK(prev);
    if (alpha_ != prev->alpha_ || offset_ != prev->offset_) {
      context->MarkSubtreeDirty(context->GetOldLayerPaintRegion(old_layer));
    }
  }
  context->PushTransform(SkMatrix::Translate(offset_.fX, offset_.fY));
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
  context->SetTransform(
      RasterCache::GetIntegralTransCTM(context->GetTransform()));
#endif
  DiffChildren(context, prev);
  context->SetLayerPaintRegion(this, context->CurrentSubtreeRegion());
}

void OpacityLayer::Preroll(PrerollContext* context, const SkMatrix& matrix) {
  TRACE_EVENT0("flutter", "OpacityLayer::Preroll");
  FML_DCHECK(!GetChildContainer()->layers().empty());  // We can't be a leaf.

  SkMatrix child_matrix = matrix;
  child_matrix.preTranslate(offset_.fX, offset_.fY);

  // Similar to what's done in TransformLayer::Preroll, we have to apply the
  // reverse transformation to the cull rect to properly cull child layers.
  context->cull_rect = context->cull_rect.makeOffset(-offset_.fX, -offset_.fY);

  // FML_LOG(ERROR) << "Opacity layer prerolling children: ";

  context->mutators_stack.PushTransform(
      SkMatrix::Translate(offset_.fX, offset_.fY));
  context->mutators_stack.PushOpacity(alpha_);
  Layer::AutoPrerollSaveLayerState save =
      Layer::AutoPrerollSaveLayerState::Create(context);
  ContainerLayer::Preroll(context, child_matrix);
  context->mutators_stack.Pop();
  context->mutators_stack.Pop();

  subtree_can_accept_opacity_ = context->subtree_can_accept_opacity;
  context->subtree_can_accept_opacity = true;

  set_paint_bounds(paint_bounds().makeOffset(offset_.fX, offset_.fY));

  // FML_LOG(ERROR) << "Opacity layer can pass opacity: " << subtree_can_accept_opacity_;

  if (!subtree_can_accept_opacity_) {
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
    child_matrix = RasterCache::GetIntegralTransCTM(child_matrix);
#endif
    TryToPrepareRasterCache(context, GetCacheableChild(), child_matrix);
  }

  // Restore cull_rect
  context->cull_rect = context->cull_rect.makeOffset(offset_.fX, offset_.fY);
}

void OpacityLayer::Paint(PaintContext& context) const {
  TRACE_EVENT0("flutter", "OpacityLayer::Paint");
  FML_DCHECK(needs_painting(context));

  SkAutoCanvasRestore save(context.internal_nodes_canvas, true);
  context.internal_nodes_canvas->translate(offset_.fX, offset_.fY);

  SkAlpha inherited_alpha = context.inherited_alpha;
  SkAlpha alpha =
      (alpha_ * inherited_alpha + SK_AlphaOPAQUE / 2) / SK_AlphaOPAQUE;

  if (subtree_can_accept_opacity_) {
    context.inherited_alpha = alpha;
    PaintChildren(context);
    context.inherited_alpha = inherited_alpha;
    return;
  }

  SkPaint paint;
  paint.setAlpha(alpha);

#ifndef SUPPORT_FRACTIONAL_TRANSLATION
  context.internal_nodes_canvas->setMatrix(RasterCache::GetIntegralTransCTM(
      context.leaf_nodes_canvas->getTotalMatrix()));
#endif

  if (context.raster_cache &&
      context.raster_cache->Draw(GetCacheableChild(),
                                 *context.leaf_nodes_canvas, &paint)) {
    return;
  }

  // Skia may clip the content with saveLayerBounds (although it's not a
  // guaranteed clip). So we have to provide a big enough saveLayerBounds. To do
  // so, we first remove the offset from paint bounds since it's already in the
  // matrix. Then we round out the bounds.
  //
  // Note that the following lines are only accessible when the raster cache is
  // not available (e.g., when we're using the software backend in golden
  // tests).
  SkRect saveLayerBounds;
  paint_bounds()
      .makeOffset(-offset_.fX, -offset_.fY)
      .roundOut(&saveLayerBounds);

  Layer::AutoSaveLayer save_layer =
      Layer::AutoSaveLayer::Create(context, saveLayerBounds, &paint);
  PaintChildren(context);
}

}  // namespace flutter
