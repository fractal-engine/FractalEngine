#ifndef MODEL_VIEWER_H
#define MODEL_VIEWER_H

#include <imgui.h>

#include "editor/gui/window_base.h"
#include "editor/pipelines/preview_pipeline.h"

class ModelViewer : public WindowBase {
public:
  explicit ModelViewer(PreviewData* data);
  ~ModelViewer() override;

  void Render() override;

private:
  void Init();

  void RenderViewport();
  void RenderModelProperties();
  void RenderInstanceList();

  PreviewPipeline preview_pipeline_;
  PreviewData* data_;
  size_t viewport_output_;

  // Camera state
  float model_pitch_ = 0.0f;
  float model_yaw_ = 0.0f;
  float camera_pan_x_ = 0.0f;
  float camera_pan_y_ = 0.0f;
  float camera_distance_ = 5.0f;
  glm::vec3 target_center_ = glm::vec3(0.0f);

  // Cached metrics
  uint32_t vert_count_ = 0;
  uint32_t tri_count_ = 0;
  uint32_t material_slots_ = 0;
  float lod_bias_ = 1.0f;

  bool initialized_ = false;
};

#endif  // MODEL_VIEWER_H
