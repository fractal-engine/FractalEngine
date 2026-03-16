#ifndef MODEL_VIEWER_H
#define MODEL_VIEWER_H

#include <imgui.h>

#include "editor/pipelines/preview_pipeline.h"
#include "platform/window_manager.h"


class ModelViewer {
public:
  ModelViewer();
  ~ModelViewer();

  void Init();
  void Render();

private:
  void RenderViewport();
  void RenderModelProperties();

  PreviewPipeline preview_pipeline_;
  size_t viewport_output_;

  // Viewport interactions
  float model_rotation_x_ = 0.0f;
  float model_rotation_y_ = 0.0f;
  float camera_distance_ = 5.0f;

  // Mock static properties
  float lod_bias_ = 1.0f;
  bool initialized_ = false;
};

#endif  // MODEL_VIEWER_H