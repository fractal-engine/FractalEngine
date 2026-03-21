#ifndef MODEL_VIEWER_H
#define MODEL_VIEWER_H

#include <imgui.h>

#include "editor/pipelines/preview_pipeline.h"
#include "platform/window_manager.h"

class Model;

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
  std::shared_ptr<Model> model_;
  

  // Viewport interactions
  float model_pitch_ = 0.0f;
  float model_yaw_ = 0.0f;
  float camera_pan_x_ = 0.0f;
  float camera_pan_y_ = 0.0f;
  float camera_distance_ = 5.0f;  // Safe starting distance
  glm::vec3 target_center_ = glm::vec3(0.0f);

  // Topology data
  uint32_t vert_count_ = 0;
  uint32_t tri_count_ = 0;
  uint32_t material_slots_ = 0;
  float lod_bias_ = 1.0f;
  bool initialized_ = false;
};

#endif  // MODEL_VIEWER_H