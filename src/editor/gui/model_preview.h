#ifndef MODEL_PREVIEW_H
#define MODEL_PREVIEW_H

#include <imgui.h>
#include <string>
#include <vector>

#include "editor/pipelines/preview_pipeline.h"
#include "platform/window_manager.h"


class ModelPreview {
public:
  ModelPreview();
  ~ModelPreview();

  void Init();
  void Render();

  // Global access to selection for the ModelViewer
  static int GetSelectedVariant();

private:
  void RenderToolbar(ImDrawList* draw_list);
  void RenderGrid(ImDrawList* draw_list);

  PreviewPipeline preview_pipeline_;
  std::vector<size_t> variant_outputs_;

  // States
  uint32_t current_seed_ = 42069;
  float thumbnail_size_ = 120.0f;

  static int s_selected_variant_;
  int total_variants_ = 48;
  bool initialized_ = false;
};

#endif  // MODEL_PREVIEW_H