#include "model_preview.h"
#include "editor/gui/components/im_components.h"
#include "imgui.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <algorithm>
#include <glm/gtx/matrix_decompose.hpp>

#include <ImGuiFileDialog/ImGuiFileDialog.h>

#include "editor/gui/styles/editor_styles.h"

#include "engine/context/engine_context.h"
#include "engine/ecs/components/transform_component.h"
#include "engine/memory/resource_manager.h"
#include "engine/pcg/procmodel/generator/model_generator.h"
#include "engine/pcg/procmodel/procmodel_resource.h"
#include "engine/renderer/model/model.h"

ModelPreview::ModelPreview(PreviewData* data) : data_(data) {}

ModelPreview::~ModelPreview() {
  if (initialized_) {
    preview_pipeline_.Destroy();
  }
}

void ModelPreview::Init() {
  if (initialized_)
    return;

  preview_pipeline_.Create();

  // Create outputs for all instances
  for (int i = 0; i < total_instances_; ++i) {
    instance_outputs_.push_back(preview_pipeline_.CreateOutput());
  }

  initialized_ = true;
}

void ModelPreview::LoadDescriptor(const std::string& path) {
  descriptor_path_ = path;

  data_->instances.clear();
  images_generated_ = 0;
  images_submitted_ = 0;

  auto& pcg = EngineContext::Generator();
  data_->archetype_id = pcg.LoadArchetype(descriptor_path_);

  if (data_->archetype_id == 0) {
    Logger::getInstance().Log(
        LogLevel::Error, "[ModelPreview] Failed to load archetype: " + path);
    data_->model = nullptr;
    return;
  }

  auto& resource_mgr = EngineContext::resourceManager();
  auto resource = resource_mgr.GetResourceAs<ProcModel::ProcModelResource>(
      data_->archetype_id);
  if (resource) {
    data_->model = Model::FromMeshData(resource->GetGraph().meshes);
  }
}

/* void ModelPreview::GenerateInstances() {
  data_->instances.clear();

  if (data_->archetype_id == 0)
    return;

  auto& resource_mgr = EngineContext::resourceManager();
  auto resource = resource_mgr.GetResourceAs<ProcModel::ProcModelResource>(
      data_->archetype_id);
  if (!resource || !resource->IsResolved())
    return;

  for (int i = 0; i < total_instances_; ++i) {
    auto resolved = ProcModel::ModelGenerator::Generate(
        resource->GetGraph(), resource->GetDescriptor(), current_seed_ + i);
    if (resolved) {
      data_->instances.push_back(std::move(*resolved));
    }
  }

  regenerate_ = false;
}*/

void ModelPreview::TickGenerate() {
  if (images_generated_ >= total_instances_)
    return;

  if (data_->archetype_id == 0)
    return;

  auto& resource_mgr = EngineContext::resourceManager();
  auto resource = resource_mgr.GetResourceAs<ProcModel::ProcModelResource>(
      data_->archetype_id);
  if (!resource || !resource->IsResolved())
    return;

  int budget = images_per_frame_;
  while (budget-- > 0 && images_generated_ < total_instances_) {
    auto resolved = ProcModel::ModelGenerator::Generate(
        resource->GetGraph(), resource->GetDescriptor(),
        current_seed_ + images_generated_, 10,
        &EngineContext::Generator().ValidationLog());
    if (resolved) {
      data_->instances.push_back(std::move(*resolved));
    }
    ++images_generated_;
  }
}

void ModelPreview::Render() {
  if (!initialized_)
    Init();

  TickGenerate();

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 20.0f));
  ImGui::Begin("Procedural Preview", nullptr);

  RenderToolbar(ImGui::GetWindowDrawList());

  ImGui::BeginChild("GridArea", ImVec2(0, 0), false);

  // Fetch the draw list INSIDE the child window so scrolling
  // automatically clips the images
  ImDrawList* child_draw_list = ImGui::GetWindowDrawList();
  RenderGrid(child_draw_list);
  ImGui::EndChild();

  ImGui::End();
  ImGui::PopStyleVar();

  SubmitViews();
  preview_pipeline_.Render();
}

void ModelPreview::RenderToolbar(ImDrawList* draw_list) {
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 8.0f));

  IMComponents::Headline("Procedural Viewer", ICON_FA_CUBES);

  if (ImGui::Button(ICON_FA_FOLDER_OPEN " Load")) {
    IGFD::FileDialogConfig cfg{};
    cfg.path = std::filesystem::current_path().string();
    IGFD::FileDialog::Instance()->OpenDialog("LoadDescriptorDlg",
                                             "Select Descriptor", ".json", cfg);
  }

  if (IGFD::FileDialog::Instance()->Display("LoadDescriptorDlg",
                                            ImGuiWindowFlags_NoCollapse,
                                            ImVec2(700.0f, 500.0f))) {
    if (IGFD::FileDialog::Instance()->IsOk()) {
      LoadDescriptor(IGFD::FileDialog::Instance()->GetFilePathName());
    }
    IGFD::FileDialog::Instance()->Close();
  }

  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_ROTATE " Generate")) {
    current_seed_ = static_cast<uint32_t>(Time::Now() * 1000.0);
    data_->instances.clear();
    images_generated_ = 0;
    images_submitted_ = 0;
  }

  // IMComponents::IndicatorLabel("Base seed: %u", current_seed_);
  ImGui::Text("Base seed: %u", current_seed_);

  ImGui::SameLine();
  float zoom_width = 100.0f;
  float current_x = ImGui::GetCursorPosX();
  float right_edge =
      ImGui::GetWindowWidth() - zoom_width - ImGui::GetStyle().WindowPadding.x;

  // Safely push to the right only if we have space, otherwise stay next in line
  if (right_edge > current_x) {
    ImGui::SetCursorPosX(right_edge);
  }

  ImGui::SetNextItemWidth(zoom_width);
  ImGui::SliderFloat("##Zoom", &thumbnail_size_, 64.0f, 120.0f, "Zoom: %.0f");

  ImGui::PopStyleVar();
  ImGui::Dummy(ImVec2(0, 5.0f));
  ImGui::Separator();
  ImGui::Dummy(ImVec2(0, 5.0f));
}

void ModelPreview::RenderGrid(ImDrawList* draw_list) {
  ImVec2 content_avail = ImGui::GetContentRegionAvail();
  float padding = 8.0f;

  int columns = std::max(
      1, static_cast<int>(content_avail.x / (thumbnail_size_ + padding)));
  float dpi = WindowManager::GetDPIScale();

  int render_count =
      std::min(total_instances_, static_cast<int>(data_->instances.size()));

  if (ImGui::BeginTable("InstanceGrid", columns)) {
    for (int i = 0; i < total_instances_; i++) {
      ImGui::TableNextColumn();

      ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
      ImVec2 rect_min = cursor_pos;
      ImVec2 rect_max =
          ImVec2(rect_min.x + thumbnail_size_, rect_min.y + thumbnail_size_);

      ImGui::InvisibleButton(("##inst_" + std::to_string(i)).c_str(),
                             ImVec2(thumbnail_size_, thumbnail_size_));
      bool hovered = ImGui::IsItemHovered();
      bool selected = (data_->selected_instance == i);

      if (ImGui::IsItemClicked()) {
        data_->selected_instance = i;
      }

      ImU32 bg_color = EditorColor::element;
      if (hovered)
        bg_color = EditorColor::element_hovered;
      if (selected)
        bg_color = EditorColor::element_active;

      draw_list->AddRectFilled(rect_min, rect_max, bg_color, 4.0f);

      bool has_instance = (i < render_count) && data_->model;

      if (has_instance) {
        size_t out_idx = instance_outputs_[i];
        preview_pipeline_.ResizeOutput(out_idx, thumbnail_size_ * dpi,
                                       thumbnail_size_ * dpi);

        bgfx::TextureHandle tex = preview_pipeline_.GetOutputTexture(out_idx);
        if (bgfx::isValid(tex)) {
          float pad = 0.5f;
          ImVec2 img_min = ImVec2(rect_min.x + pad, rect_min.y + pad);
          ImVec2 img_max = ImVec2(rect_max.x - pad, rect_max.y - pad);
          draw_list->AddImage((ImTextureID)(uintptr_t)tex.idx, img_min, img_max,
                              ImVec2(0, 1), ImVec2(1, 0));
        }
      } else {
        ImGui::PushFont(EditorStyles::GetFonts().h2);
        std::string icon = ICON_FA_CUBE;
        ImVec2 text_size = ImGui::CalcTextSize(icon.c_str());
        draw_list->AddText(
            ImVec2(rect_min.x + (thumbnail_size_ - text_size.x) * 0.5f,
                   rect_min.y + (thumbnail_size_ - text_size.y) * 0.5f),
            EditorColor::text, icon.c_str());
        ImGui::PopFont();
      }

      draw_list->AddRect(rect_min, rect_max, EditorColor::border_color, 4.0f, 0,
                         1.0f);
    }
    ImGui::EndTable();
  }
}

void ModelPreview::SubmitViews() {
  if (!data_->model)
    return;

  float dpi = WindowManager::GetDPIScale();
  int render_count =
      std::min(total_instances_, static_cast<int>(data_->instances.size()));

  // Apply group coloring from group ID - REMOVE THIS
  static const std::unordered_map<std::string, glm::vec3> GROUP_COLORS = {
      {"_BASE_", glm::vec3(0.92f, 0.92f, 0.90f)},  // white

      // red
      {"_ROOF_A", glm::vec3(0.68f, 0.28f, 0.28f)},
      {"_ROOF_B", glm::vec3(0.80f, 0.38f, 0.38f)},
      {"_ROOF_C", glm::vec3(0.90f, 0.50f, 0.50f)},

      // blue
      {"_WINDOW_A", glm::vec3(0.35f, 0.50f, 0.85f)},
      {"_WINDOW_B", glm::vec3(0.50f, 0.65f, 0.92f)},
      {"_WINDOW_C", glm::vec3(0.65f, 0.78f, 0.97f)},

      // green
      {"_PILLAR_A", glm::vec3(0.40f, 0.70f, 0.50f)},
      {"_PILLAR_B", glm::vec3(0.55f, 0.82f, 0.65f)},
      {"_PILLAR_C", glm::vec3(0.70f, 0.90f, 0.78f)},

      // yellow
      {"_CHIMNEY_A", glm::vec3(0.78f, 0.68f, 0.30f)},
      {"_CHIMNEY_B", glm::vec3(0.88f, 0.78f, 0.40f)},
      {"_CHIMNEY_C", glm::vec3(0.95f, 0.86f, 0.55f)},

      // purple
      {"_DECOR_A", glm::vec3(0.68f, 0.45f, 0.78f)},
      {"_DECOR_B", glm::vec3(0.80f, 0.60f, 0.88f)},
      {"_DECOR_C", glm::vec3(0.90f, 0.72f, 0.95f)},
  };

  int budget = images_per_frame_;

  while (images_submitted_ < render_count && budget-- > 0) {
    int i = images_submitted_;
    size_t out_idx = instance_outputs_[i];
    preview_pipeline_.ResizeOutput(out_idx, thumbnail_size_ * dpi,
                                   thumbnail_size_ * dpi);

    bool on_first_draw = true;

    // Submit per-part render instructions
    for (const auto& desc : data_->instances[i].descriptors) {
      auto color_it = GROUP_COLORS.find(desc.group_id);  // ! REMOVE THIS

      // ! REMOVE THIS
      glm::vec3 color = (color_it != GROUP_COLORS.end())
                            ? color_it->second
                            : glm::vec3(0.7f);  // Default grey fallback

      for (int idx : desc.mesh_indices) {
        uint32_t mesh_idx = static_cast<uint32_t>(idx);

        PreviewRenderInstruction inst;
        inst.output_index = out_idx;
        inst.background_color = glm::vec4(0.35f, 0.35f, 0.35f, 1.0f);
        inst.model = data_->model.get();
        inst.clear_output = on_first_draw;
        on_first_draw = false;

        inst.mesh_filter = {mesh_idx};
        inst.mesh_colors[mesh_idx] = color;  // ! REMOVE THIS

        // Use resolved transform
        glm::vec3 pos, scale, skew;
        glm::quat rot;
        glm::vec4 persp;
        glm::decompose(desc.local_transform, scale, rot, pos, skew, persp);

        TransformComponent transform_component;
        transform_component.position_ = pos;
        transform_component.rotation_ = rot;
        transform_component.scale_ = scale;
        inst.mesh_transforms[mesh_idx] = transform_component;

        inst.camera_transform.position_ = glm::vec3(0.0f, 15.0f, -60.0f);

        preview_pipeline_.AddRenderInstruction(inst);
      }
    }
    ++images_submitted_;
  }
}
