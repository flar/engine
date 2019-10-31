// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/compositor_context.h"

#include "flutter/flow/layers/layer_tree.h"
#include "third_party/skia/include/core/SkCanvas.h"

namespace flutter {

CompositorContext::CompositorContext(fml::Milliseconds frame_budget)
    : raster_time_(frame_budget), ui_time_(frame_budget) {}

CompositorContext::~CompositorContext() = default;

void CompositorContext::BeginFrame(ScopedFrame& frame,
                                   bool enable_instrumentation) {
  if (enable_instrumentation) {
    frame_count_.Increment();
    raster_time_.Start();
  }
}

void CompositorContext::EndFrame(ScopedFrame& frame,
                                 bool enable_instrumentation) {
  raster_cache_.SweepAfterFrame();
  if (enable_instrumentation) {
    raster_time_.Stop();
  }
}

std::unique_ptr<CompositorContext::ScopedFrame> CompositorContext::AcquireFrame(
    GrContext* gr_context,
    SkCanvas* canvas,
    SkIRect udpate_bounds,
    ExternalViewEmbedder* view_embedder,
    const SkMatrix& root_surface_transformation,
    bool instrumentation_enabled,
    fml::RefPtr<fml::GpuThreadMerger> gpu_thread_merger) {
  return std::make_unique<ScopedFrame>(
      *this, gr_context, canvas, udpate_bounds, view_embedder, root_surface_transformation,
      instrumentation_enabled, gpu_thread_merger);
}

CompositorContext::ScopedFrame::ScopedFrame(
    CompositorContext& context,
    GrContext* gr_context,
    SkCanvas* canvas,
    SkIRect update_bounds,
    ExternalViewEmbedder* view_embedder,
    const SkMatrix& root_surface_transformation,
    bool instrumentation_enabled,
    fml::RefPtr<fml::GpuThreadMerger> gpu_thread_merger)
    : context_(context),
      gr_context_(gr_context),
      canvas_(canvas),
      update_bounds_(update_bounds),
      view_embedder_(view_embedder),
      root_surface_transformation_(root_surface_transformation),
      instrumentation_enabled_(instrumentation_enabled),
      gpu_thread_merger_(gpu_thread_merger) {
  context_.BeginFrame(*this, instrumentation_enabled_);
}

CompositorContext::ScopedFrame::~ScopedFrame() {
  context_.EndFrame(*this, instrumentation_enabled_);
}

RasterStatus CompositorContext::ScopedFrame::Raster(
    flutter::LayerTree& layer_tree,
    bool ignore_raster_cache) {
  SkRect dirty_rect = layer_tree.Preroll(*this, ignore_raster_cache);
  update_bounds_.join(dirty_rect.roundOut());
  dirty_rect.set(update_bounds_);
  bool update_all = false;
  PostPrerollResult post_preroll_result = PostPrerollResult::kSuccess;
  if (view_embedder_ && gpu_thread_merger_) {
    post_preroll_result = view_embedder_->PostPrerollAction(gpu_thread_merger_);
  }

  if (post_preroll_result == PostPrerollResult::kResubmitFrame) {
    return RasterStatus::kResubmit;
  }
  // Clearing canvas after preroll reduces one render target switch when preroll
  // paints some raster cache.
  if (canvas()) {
    FML_LOG(INFO) << "Rendering to "
        << dirty_rect.left() << ", " << dirty_rect.top() << " => "
        << dirty_rect.right() << ", " << dirty_rect.bottom();
    if (!update_all) {
      canvas()->save();
      canvas()->clipRect(dirty_rect, false);
    }
    canvas()->clear(SK_ColorTRANSPARENT);
  }
  layer_tree.Paint(*this, ignore_raster_cache);
  if (!update_all && canvas()) {
    canvas()->restore();
    // SkPaint p;
    // p.setColor(SK_ColorRED);
    // p.setStyle(SkPaint::kStroke_Style);
    // p.setAlphaf(0.25f);
    // p.setStrokeWidth(4);
    // canvas()->drawRect(dirty_rect, p);
  }
  return RasterStatus::kSuccess;
}

void CompositorContext::OnGrContextCreated() {
  texture_registry_.OnGrContextCreated();
  raster_cache_.Clear();
}

void CompositorContext::OnGrContextDestroyed() {
  texture_registry_.OnGrContextDestroyed();
  raster_cache_.Clear();
}

}  // namespace flutter
