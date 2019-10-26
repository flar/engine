// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/lib/ui/compositing/scene_builder.h"

#include "flutter/flow/layers/backdrop_filter_layer.h"
#include "flutter/flow/layers/clip_path_layer.h"
#include "flutter/flow/layers/clip_rect_layer.h"
#include "flutter/flow/layers/clip_rrect_layer.h"
#include "flutter/flow/layers/color_filter_layer.h"
#include "flutter/flow/layers/container_layer.h"
#include "flutter/flow/layers/layer.h"
#include "flutter/flow/layers/layer_tree.h"
#include "flutter/flow/layers/opacity_layer.h"
#include "flutter/flow/layers/performance_overlay_layer.h"
#include "flutter/flow/layers/physical_shape_layer.h"
#include "flutter/flow/layers/picture_layer.h"
#include "flutter/flow/layers/platform_view_layer.h"
#include "flutter/flow/layers/shader_mask_layer.h"
#include "flutter/flow/layers/texture_layer.h"
#include "flutter/flow/layers/transform_layer.h"
#include "flutter/fml/build_config.h"
#include "flutter/lib/ui/painting/matrix.h"
#include "flutter/lib/ui/painting/shader.h"
#include "flutter/lib/ui/ui_dart_state.h"
#include "flutter/lib/ui/window/window.h"
#include "third_party/skia/include/core/SkColorFilter.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "third_party/tonic/dart_args.h"
#include "third_party/tonic/dart_binding_macros.h"
#include "third_party/tonic/dart_library_natives.h"

#if defined(OS_FUCHSIA)
#include "flutter/flow/layers/child_scene_layer.h"
#endif

namespace flutter {

static void SceneBuilder_constructor(Dart_NativeArguments args) {
  DartCallConstructor(&SceneBuilder::create, args);
}

IMPLEMENT_WRAPPERTYPEINFO(ui, SceneBuilder);

#define FOR_EACH_BINDING(V)                         \
  V(SceneBuilder, pushOffset)                       \
  V(SceneBuilder, pushTransform)                    \
  V(SceneBuilder, pushClipRect)                     \
  V(SceneBuilder, pushClipRRect)                    \
  V(SceneBuilder, pushClipPath)                     \
  V(SceneBuilder, pushOpacity)                      \
  V(SceneBuilder, pushColorFilter)                  \
  V(SceneBuilder, pushBackdropFilter)               \
  V(SceneBuilder, pushShaderMask)                   \
  V(SceneBuilder, pushPhysicalShape)                \
  V(SceneBuilder, pop)                              \
  V(SceneBuilder, addPlatformView)                  \
  V(SceneBuilder, addRetained)                      \
  V(SceneBuilder, addPicture)                       \
  V(SceneBuilder, addTexture)                       \
  V(SceneBuilder, addPerformanceOverlay)            \
  V(SceneBuilder, setRasterizerTracingThreshold)    \
  V(SceneBuilder, setCheckerboardOffscreenLayers)   \
  V(SceneBuilder, setCheckerboardRasterCacheImages) \
  V(SceneBuilder, build)

FOR_EACH_BINDING(DART_NATIVE_CALLBACK)
#if defined(OS_FUCHSIA)
DART_NATIVE_CALLBACK(SceneBuilder, addChildScene)
#endif

void SceneBuilder::RegisterNatives(tonic::DartLibraryNatives* natives) {
  natives->Register({
    {"SceneBuilder_constructor", SceneBuilder_constructor, 1, true},
        FOR_EACH_BINDING(DART_REGISTER_NATIVE)
#if defined(OS_FUCHSIA)
            DART_REGISTER_NATIVE(SceneBuilder, addChildScene)
#endif
  });
}

SceneBuilder::SceneBuilder() = default;
SceneBuilder::~SceneBuilder() = default;

#define AS_LAYER(type, oldLayer)                                         \
  (oldLayer ? std::static_pointer_cast<flutter::type>(oldLayer->Layer()) \
            : nullptr)

fml::RefPtr<EngineLayer> SceneBuilder::pushTransform(
    tonic::Float64List& matrix4,
    fml::RefPtr<EngineLayer> oldLayer) {
  SkMatrix sk_matrix = ToSkMatrix(matrix4);
  auto layer = flutter::TransformLayer::makeLayer(
      sk_matrix, AS_LAYER(TransformLayer, oldLayer));
  PushLayer(layer);
  // matrix4 has to be released before we can return another Dart object
  matrix4.Release();
  return EngineLayer::MakeRetained(layer);
}

fml::RefPtr<EngineLayer> SceneBuilder::pushOffset(
    double dx,
    double dy,
    fml::RefPtr<EngineLayer> oldLayer) {
  SkMatrix sk_matrix = SkMatrix::MakeTrans(dx, dy);
  auto layer = flutter::TransformLayer::makeLayer(
      sk_matrix, AS_LAYER(TransformLayer, oldLayer));
  PushLayer(layer);
  return EngineLayer::MakeRetained(layer);
}

fml::RefPtr<EngineLayer> SceneBuilder::pushClipRect(
    double left,
    double right,
    double top,
    double bottom,
    int clipBehavior,
    fml::RefPtr<EngineLayer> oldLayer) {
  SkRect clipRect = SkRect::MakeLTRB(left, top, right, bottom);
  flutter::Clip clip_behavior = static_cast<flutter::Clip>(clipBehavior);
  auto layer = flutter::ClipRectLayer::makeLayer(
      clipRect, clip_behavior, AS_LAYER(ClipRectLayer, oldLayer));
  PushLayer(layer);
  return EngineLayer::MakeRetained(layer);
}

fml::RefPtr<EngineLayer> SceneBuilder::pushClipRRect(
    const RRect& rrect,
    int clipBehavior,
    fml::RefPtr<EngineLayer> oldLayer) {
  flutter::Clip clip_behavior = static_cast<flutter::Clip>(clipBehavior);
  auto layer = flutter::ClipRRectLayer::makeLayer(
      rrect.sk_rrect, clip_behavior, AS_LAYER(ClipRRectLayer, oldLayer));
  PushLayer(layer);
  return EngineLayer::MakeRetained(layer);
}

fml::RefPtr<EngineLayer> SceneBuilder::pushClipPath(
    const CanvasPath* path,
    int clipBehavior,
    fml::RefPtr<EngineLayer> oldLayer) {
  flutter::Clip clip_behavior = static_cast<flutter::Clip>(clipBehavior);
  FML_DCHECK(clip_behavior != flutter::Clip::none);
  auto layer = flutter::ClipPathLayer::makeLayer(
      path->path(), clip_behavior, AS_LAYER(ClipPathLayer, oldLayer));
  PushLayer(layer);
  return EngineLayer::MakeRetained(layer);
}

fml::RefPtr<EngineLayer> SceneBuilder::pushOpacity(
    int alpha,
    double dx,
    double dy,
    fml::RefPtr<EngineLayer> oldLayer) {
  auto layer = flutter::OpacityLayer::makeLayer(
      alpha, SkPoint::Make(dx, dy), AS_LAYER(OpacityLayer, oldLayer));
  PushLayer(layer);
  return EngineLayer::MakeRetained(layer);
}

fml::RefPtr<EngineLayer> SceneBuilder::pushColorFilter(
    const ColorFilter* color_filter,
    fml::RefPtr<EngineLayer> oldLayer) {
  auto layer = flutter::ColorFilterLayer::makeLayer(
      color_filter->filter(), AS_LAYER(ColorFilterLayer, oldLayer));
  PushLayer(layer);
  return EngineLayer::MakeRetained(layer);
}

fml::RefPtr<EngineLayer> SceneBuilder::pushBackdropFilter(
    ImageFilter* filter,
    fml::RefPtr<EngineLayer> oldLayer) {
  auto layer = flutter::BackdropFilterLayer::makeLayer(
      filter->filter(), AS_LAYER(BackdropFilterLayer, oldLayer));
  PushLayer(layer);
  return EngineLayer::MakeRetained(layer);
}

fml::RefPtr<EngineLayer> SceneBuilder::pushShaderMask(
    Shader* shader,
    double maskRectLeft,
    double maskRectRight,
    double maskRectTop,
    double maskRectBottom,
    int blendMode,
    fml::RefPtr<EngineLayer> oldLayer) {
  SkRect rect = SkRect::MakeLTRB(maskRectLeft, maskRectTop, maskRectRight,
                                 maskRectBottom);
  auto layer = flutter::ShaderMaskLayer::makeLayer(
      shader->shader(), rect, static_cast<SkBlendMode>(blendMode),
      AS_LAYER(ShaderMaskLayer, oldLayer));
  PushLayer(layer);
  return EngineLayer::MakeRetained(layer);
}

fml::RefPtr<EngineLayer> SceneBuilder::pushPhysicalShape(
    const CanvasPath* path,
    double elevation,
    int color,
    int shadow_color,
    int clipBehavior,
    fml::RefPtr<EngineLayer> oldLayer) {
  auto layer = flutter::PhysicalShapeLayer::makeLayer(
      static_cast<SkColor>(color), static_cast<SkColor>(shadow_color),
      static_cast<float>(UIDartState::Current()
                             ->window()
                             ->viewport_metrics()
                             .device_pixel_ratio),
      static_cast<float>(
          UIDartState::Current()->window()->viewport_metrics().physical_depth),
      static_cast<float>(elevation), path->path(),
      static_cast<flutter::Clip>(clipBehavior),
      AS_LAYER(PhysicalShapeLayer, oldLayer));
  PushLayer(layer);
  return EngineLayer::MakeRetained(layer);
}

void SceneBuilder::addRetained(fml::RefPtr<EngineLayer> retainedLayer) {
  if (!current_layer_) {
    return;
  }
  current_layer_->Add(retainedLayer->Layer());
}

void SceneBuilder::pop() {
  if (!current_layer_) {
    return;
  }
  current_layer_ = current_layer_->parent();
}

void SceneBuilder::addPicture(double dx,
                              double dy,
                              Picture* picture,
                              int hints) {
  if (!current_layer_) {
    return;
  }
  SkPoint offset = SkPoint::Make(dx, dy);
  SkRect pictureRect = picture->picture()->cullRect();
  pictureRect.offset(offset.x(), offset.y());
  auto layer = std::make_unique<flutter::PictureLayer>(
      offset, UIDartState::CreateGPUObject(picture->picture()), !!(hints & 1),
      !!(hints & 2));
  current_layer_->Add(std::move(layer));
}

void SceneBuilder::addTexture(double dx,
                              double dy,
                              double width,
                              double height,
                              int64_t textureId,
                              bool freeze) {
  if (!current_layer_) {
    return;
  }
  auto layer = std::make_unique<flutter::TextureLayer>(
      SkPoint::Make(dx, dy), SkSize::Make(width, height), textureId, freeze);
  current_layer_->Add(std::move(layer));
}

void SceneBuilder::addPlatformView(double dx,
                                   double dy,
                                   double width,
                                   double height,
                                   int64_t viewId) {
  if (!current_layer_) {
    return;
  }
  auto layer = std::make_unique<flutter::PlatformViewLayer>(
      SkPoint::Make(dx, dy), SkSize::Make(width, height), viewId);
  current_layer_->Add(std::move(layer));
}

#if defined(OS_FUCHSIA)
void SceneBuilder::addChildScene(double dx,
                                 double dy,
                                 double width,
                                 double height,
                                 SceneHost* sceneHost,
                                 bool hitTestable) {
  if (!current_layer_) {
    return;
  }
  auto layer = std::make_unique<flutter::ChildSceneLayer>(
      sceneHost->id(), SkPoint::Make(dx, dy), SkSize::Make(width, height),
      hitTestable);
  current_layer_->Add(std::move(layer));
}
#endif  // defined(OS_FUCHSIA)

void SceneBuilder::addPerformanceOverlay(uint64_t enabledOptions,
                                         double left,
                                         double right,
                                         double top,
                                         double bottom) {
  if (!current_layer_) {
    return;
  }
  SkRect rect = SkRect::MakeLTRB(left, top, right, bottom);
  auto layer =
      std::make_unique<flutter::PerformanceOverlayLayer>(enabledOptions);
  layer->set_paint_bounds(rect);
  current_layer_->Add(std::move(layer));
}

void SceneBuilder::setRasterizerTracingThreshold(uint32_t frameInterval) {
  rasterizer_tracing_threshold_ = frameInterval;
}

void SceneBuilder::setCheckerboardRasterCacheImages(bool checkerboard) {
  checkerboard_raster_cache_images_ = checkerboard;
}

void SceneBuilder::setCheckerboardOffscreenLayers(bool checkerboard) {
  checkerboard_offscreen_layers_ = checkerboard;
}

fml::RefPtr<Scene> SceneBuilder::build() {
  fml::RefPtr<Scene> scene = Scene::create(
      std::move(root_layer_), rasterizer_tracing_threshold_,
      checkerboard_raster_cache_images_, checkerboard_offscreen_layers_);
  ClearDartWrapper();
  return scene;
}

void SceneBuilder::PushLayer(std::shared_ptr<flutter::ContainerLayer> layer) {
  FML_DCHECK(layer);

  if (!root_layer_) {
    root_layer_ = std::move(layer);
    current_layer_ = root_layer_.get();
    return;
  }

  if (!current_layer_) {
    return;
  }

  flutter::ContainerLayer* newLayer = layer.get();
  current_layer_->Add(std::move(layer));
  current_layer_ = newLayer;
}

}  // namespace flutter
