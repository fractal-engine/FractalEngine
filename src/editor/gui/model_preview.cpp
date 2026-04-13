#include "model_preview.h"

#include <algorithm>

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

  auto& pcg = EngineContext::Generator();
  data_->archetype_id = pcg.LoadArchetype(descriptor_path_);

  if (data_->archetype_id == 0) {
    Logger::getInstance().Log(
        LogLevel::Error, "[ModelPreview] Failed to load archetype: " + path);
    data_->model = nullptr;
    data_->instances.clear();
    return;
  }

  auto& resource_mgr = EngineContext::resourceManager();
  auto resource = resource_mgr.GetResourceAs<ProcModel::ProcModelResource>(
      data_->archetype_id);
  if (resource) {
    data_->model = Model::FromMeshData(resource->GetGraph().meshes);
  }
  regenerate_ = true;
}

void ModelPreview::GenerateInstances() {
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
}

void ModelPreview::Render() {
  if (!initialized_)
    Init();

  if (regenerate_)
    GenerateInstances();

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
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

  preview_pipeline_.Render();
}

void ModelPreview::RenderToolbar(ImDrawList* draw_list) {
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));

  if (ImGui::Button(ICON_FA_FOLDER_OPEN " Load")) {
    IGFD::FileDialogConfig cfg{};
    cfg.path = std::filesystem::current_path().string();
    IGFD::FileDialog::Instance()->OpenDialog("LoadDescriptorDlg",
                                             "Select Descriptor", ".json", cfg);
  }

  if (IGFD::FileDialog::Instance()->Display("LoadDescriptorDlg")) {
    if (IGFD::FileDialog::Instance()->IsOk()) {
      LoadDescriptor(IGFD::FileDialog::Instance()->GetFilePathName());
    }
    IGFD::FileDialog::Instance()->Close();
  }

  if (ImGui::Button(ICON_FA_ROTATE " Generate")) {
    current_seed_ = static_cast<uint32_t>(Time::Now() * 1000.0);
    regenerate_ = true;
  }

  ImGui::SameLine();
  ImGui::Text("Base seed: %u", current_seed_);

  ImGui::SameLine();
  float zoom_width = 120.0f;
  float current_x = ImGui::GetCursorPosX();
  float right_edge =
      ImGui::GetWindowWidth() - zoom_width - ImGui::GetStyle().WindowPadding.x;

  // Safely push to the right only if we have space, otherwise stay next in line
  if (right_edge > current_x) {
    ImGui::SetCursorPosX(right_edge);
  }

  ImGui::SetNextItemWidth(zoom_width);
  ImGui::SliderFloat("##Zoom", &thumbnail_size_, 64.0f, 256.0f, "Zoom: %.0f");

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

        PreviewRenderInstruction inst;
        inst.output_index = out_idx;
        inst.background_color = glm::vec4(0.06f, 0.06f, 0.06f, 1.0f);
        inst.model = data_->model.get();

        for (const auto& desc : data_->instances[i].descriptors) {
          for (int idx : desc.mesh_indices) {
            inst.mesh_filter.push_back(static_cast<uint32_t>(idx));
          }
        }

        inst.model_transform.scale_ = glm::vec3(1.0f);
        inst.model_transform.rotation_ =
            glm::angleAxis(0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        inst.camera_transform.position_ = glm::vec3(0.0f, 0.0f, -6.0f);

        preview_pipeline_.AddRenderInstruction(inst);

        bgfx::TextureHandle tex = preview_pipeline_.GetOutputTexture(out_idx);
        if (bgfx::isValid(tex)) {
          float pad = 3.0f;
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
