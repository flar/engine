// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/layers/picture_layer.h"

#include "flutter/fml/logging.h"

namespace flutter {

PictureLayer::PictureLayer(const SkPoint& offset,
                           SkiaGPUObject<SkPicture> picture,
                           bool is_complex,
                           bool will_change)
    : offset_(offset),
      picture_(std::move(picture)),
      is_complex_(is_complex),
      will_change_(will_change) {}

PictureLayer::~PictureLayer() = default;

SkData* PictureLayer::get_data() {
  SkData* the_data = data_.get();
  if (!the_data) {
    data_ = picture()->serialize();
    the_data = data_.get();
  }
  return the_data;
}

bool PictureLayer::compare_picture(PictureLayer* other_picture) {
  SkPicture* a = picture();
  SkPicture* b = other_picture->picture();
  if (a->uniqueID() == b->uniqueID()) {
    return true;
  }
  if (a->cullRect() != b->cullRect() ||
      a->approximateOpCount() != b->approximateOpCount() ||
      a->approximateBytesUsed() != b->approximateBytesUsed()) {
    return false;
  }
  return get_data()->equals(other_picture->get_data());
}

bool PictureLayer::can_replace(Layer* other) {
  PictureLayer* other_picture = other->as_picture_layer();
  if (other_picture) {
    if (other_picture->offset_ == this->offset_ &&
        // is_complex and will_change are strategy properties
        // and do not affect the rasterized output
        // other_picture->is_complex_ == this->is_complex_ &&
        // other_picture->will_change_ == this->will_change_ &&
        compare_picture(other_picture)) {
      set_painted(other_picture->is_painted());
    }
    return true;
  }
  FML_LOG(ERROR) << "PictureLayer replacing some other kind of layer: "
      << this->picture()->uniqueID()
      << " @ " << offset_.x() << ", " << offset_.y();
  return false;
}

void PictureLayer::Preroll(PrerollContext* context, const SkMatrix& matrix) {
  SkPicture* sk_picture = picture();

  if (auto* cache = context->raster_cache) {
    SkMatrix ctm = matrix;
    ctm.postTranslate(offset_.x(), offset_.y());
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
    ctm = RasterCache::GetIntegralTransCTM(ctm);
#endif
    cache->Prepare(context->gr_context, sk_picture, ctm,
                   context->dst_color_space, is_complex_, will_change_);
  }

  SkRect bounds = sk_picture->cullRect().makeOffset(offset_.x(), offset_.y());
  set_paint_bounds(bounds);
}

void PictureLayer::Paint(PaintContext& context) const {
  TRACE_EVENT0("flutter", "PictureLayer::Paint");
  FML_DCHECK(picture_.get());
  FML_DCHECK(needs_painting());

  SkAutoCanvasRestore save(context.leaf_nodes_canvas, true);
  context.leaf_nodes_canvas->translate(offset_.x(), offset_.y());
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
  context.leaf_nodes_canvas->setMatrix(RasterCache::GetIntegralTransCTM(
      context.leaf_nodes_canvas->getTotalMatrix()));
#endif

  if (context.raster_cache) {
    const SkMatrix& ctm = context.leaf_nodes_canvas->getTotalMatrix();
    RasterCacheResult result = context.raster_cache->Get(*picture(), ctm);
    if (result.is_valid()) {
      result.draw(*context.leaf_nodes_canvas);
      return;
    }
  }
  context.leaf_nodes_canvas->drawPicture(picture());
}

}  // namespace flutter
