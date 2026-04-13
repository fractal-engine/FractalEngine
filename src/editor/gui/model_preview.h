#ifndef MODEL_PREVIEW_H
#define MODEL_PREVIEW_H

#include <imgui.h>
#include <string>
#include <vector>

#include "editor/gui/window_base.h"
#include "editor/pipelines/preview_pipeline.h"
#include "engine/memory/resource.h"
#include "engine/pcg/procmodel/generator/resolved_model.h"
#include "platform/window_manager.h"

class Model;  // Forward declare model

class ModelPreview : public WindowBase {
public:
  explicit ModelPreview(PreviewData* data);
  ~ModelPreview() override;

  void Render() override;

private:
  void Init();

  void LoadDescriptor(const std::string& path);
  void GenerateInstances();

  void RenderToolbar(ImDrawList* draw_list);
  void RenderGrid(ImDrawList* draw_list);

  PreviewPipeline preview_pipeline_;
  std::vector<size_t> instance_outputs_;
  PreviewData* data_;

  // Procmodel state
  ResourceID archetype_id_ = 0;
  std::string descriptor_path_;
  std::vector<ProcModel::ResolvedModel> instances_;

  // UI States
  uint32_t current_seed_;  // seed is only defined in procmodel, remove this
  float thumbnail_size_ = 120.0f;
  int total_instances_ = 48;
  bool initialized_ = false;
  bool regenerate_ = false;
  int selected_instance_;
};

#endif  // MODEL_PREVIEW_H
