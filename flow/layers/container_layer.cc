// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/layers/container_layer.h"

namespace flutter {

ContainerLayer::ContainerLayer() {}

ContainerLayer::~ContainerLayer() = default;

void ContainerLayer::Add(std::shared_ptr<Layer> layer) {
  layer->set_parent(this);
  layers_.push_back(std::move(layer));
}

void ContainerLayer::Preroll(PrerollContext* context, const SkMatrix& matrix) {
  TRACE_EVENT0("flutter", "ContainerLayer::Preroll");

  SkRect child_paint_bounds = SkRect::MakeEmpty();
  PrerollChildren(context, matrix, &child_paint_bounds);
  set_paint_bounds(child_paint_bounds);
}

void ContainerLayer::PrerollChildren(PrerollContext* context,
                                     const SkMatrix& child_matrix,
                                     SkRect* child_paint_bounds) {
  int first_changed, end, last_cur, last_prev;
  end = layers_.size();
  if (check_children_) {
    check_children_ = false;
    first_changed = 0;
    last_cur = end;
    last_prev = prev_layers_.size();
    while (first_changed < last_cur &&
           first_changed < last_prev &&
           layers_[first_changed]->can_replace(prev_layers_[first_changed].get())) {
      first_changed++;
    }
    while (first_changed < last_cur-1 &&
           first_changed < last_prev-1 &&
           layers_[last_cur-1]->can_replace(prev_layers_[last_prev-1].get())) {
      last_cur--;
      last_prev--;
    }
    if (first_changed < end || first_changed < last_prev) {
      FML_LOG(ERROR) << "Children changed"
          << "[ 0 => " << first_changed << " => " << last_prev << " => " << prev_layers_.size() << "] => "
          << "[ 0 => " << first_changed << " => " << last_cur << " => " << end << "]";
    }
  } else {
    first_changed = last_cur = last_prev = end;
  }
  // [first_changed, last_cur) are new in current layers_
  // [first_changed, last_prev) are new in prev_layers_

  // Platform views have no children, so context->has_platform_view should
  // always be false.
  FML_DCHECK(!context->has_platform_view);
  bool child_has_platform_view = false;

  for (int i = 0; i < end; i++) {
    auto* layer = layers_[i].get();

    bool was_painted = layer->is_painted();
    if (i == first_changed && i < last_prev) {
      if (is_painted()) {
        FML_LOG(ERROR) << *this << " is now dirty due to missing " << (last_prev - i) << " children at " << i;
      }
      // set_painted(false);
      for (int j = i; j < last_prev; j++) {
        FML_LOG(ERROR) << "Missing Old layer: " << *prev_layers_[j] << " is dirty";
        context->dirty_rect.join(prev_layers_[j]->paint_bounds());
      }
      last_prev = first_changed;
    }

    // Reset context->has_platform_view to false so that layers aren't treated
    // as if they have a platform view based on one being previously found in a
    // sibling tree.
    context->has_platform_view = false;

    layer->Preroll(context, child_matrix);

    if (layer->needs_system_composite()) {
      set_needs_system_composite(true);
    }
    child_paint_bounds->join(layer->paint_bounds());

    child_has_platform_view =
        child_has_platform_view || context->has_platform_view;

    if (!layer->is_painted() || (i >= first_changed && i < last_cur)) {
      if (i >= first_changed && i < last_cur) {
        if (layer->is_painted()) {
          FML_LOG(ERROR) << "Inserted layer: " << *layer << " was not otherwise dirty";
        } else {
          FML_LOG(ERROR) << "Inserted layer: " << *layer << " is dirty";
        }
      } else if (was_painted) {
        FML_LOG(ERROR) << "Old layer: " << *layer << " is dirty after preroll";
      } else {
        FML_LOG(ERROR) << "New layer: " << *layer << " is dirty";
      }
      if (is_painted()) {
        FML_LOG(ERROR) << *this << " is now dirty due to dirty child (" << *layer << ") at " << i;
      }
      // set_painted(false);
      context->dirty_rect.join(layer->paint_bounds());
    }
  }

  context->has_platform_view = child_has_platform_view;

  if (first_changed < last_prev) {
    if (is_painted()) {
      FML_LOG(ERROR) << *this << " is now dirty due to " << (last_prev - first_changed) << " children trimmed after " << first_changed;
    }
    // set_painted(false);
    for (int j = first_changed; j < last_prev; j++) {
      FML_LOG(ERROR) << "Extra Old layer: " << *prev_layers_[j] << " is dirty";
      context->dirty_rect.join(prev_layers_[j]->paint_bounds());
    }
  }
  prev_layers_.clear();
}

void ContainerLayer::PaintChildren(PaintContext& context) const {
  FML_DCHECK(needs_painting());

  // Intentionally not tracing here as there should be no self-time
  // and the trace event on this common function has a small overhead.
  for (auto& layer : layers_) {
    if (layer->needs_painting()) {
      layer->Paint(context);
    }
    layer->set_painted(true);
  }
}

#if defined(OS_FUCHSIA)

void ContainerLayer::UpdateScene(SceneUpdateContext& context) {
  UpdateSceneChildren(context);
}

void ContainerLayer::UpdateSceneChildren(SceneUpdateContext& context) {
  FML_DCHECK(needs_system_composite());

  // Paint all of the layers which need to be drawn into the container.
  // These may be flattened down to a containing
  for (auto& layer : layers_) {
    if (layer->needs_system_composite()) {
      layer->UpdateScene(context);
    }
  }
}

#endif  // defined(OS_FUCHSIA)

}  // namespace flutter
