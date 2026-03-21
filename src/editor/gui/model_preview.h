#ifndef MODEL_PREVIEW_H
#define MODEL_PREVIEW_H

#include <imgui.h>
#include <string>
#include <vector>

#include "editor/pipelines/preview_pipeline.h"
#include "platform/window_manager.h"

class Model;  // Forward declare model

class ModelPreview {
public:
  ModelPreview();
  ~ModelPreview();

  void Init();
  void Render();

  // Global access to selection for the ModelViewer
  static int GetSelectedVariant();

  // Expose the loaded model so the Viewer doesn't have to reload it
  static std::shared_ptr<Model> GetModel();
  static float GetModelScale();  // Allows Viewer to match the grid scale

private:
  void RenderToolbar(ImDrawList* draw_list);
  void RenderGrid(ImDrawList* draw_list);

  PreviewPipeline preview_pipeline_;
  std::vector<size_t> variant_outputs_;

  // The base procedural model being visualized
  static std::shared_ptr<Model> model_;

  // States
  uint32_t current_seed_ = 42069;
  float thumbnail_size_ = 120.0f;
  static int s_selected_variant_;
  static float s_model_scale_;
  int total_variants_ = 48;
  bool initialized_ = false;
};

#endif  // MODEL_PREVIEW_H