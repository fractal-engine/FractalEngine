#include "model_preview.h"

#include <algorithm>
#include "editor/gui/styles/editor_styles.h"
#include "engine/ecs/components/transform_component.h"  // For TransformComponent

int ModelPreview::s_selected_variant_ = 0;

int ModelPreview::GetSelectedVariant() {
  return s_selected_variant_;
}

ModelPreview::ModelPreview() {}

ModelPreview::~ModelPreview() {
  if (initialized_) {
    preview_pipeline_.Destroy();
  }
}

void ModelPreview::Init() {
  if (initialized_)
    return;

  preview_pipeline_.Create();

  // Create outputs for all our procedural variants
  for (int i = 0; i < total_variants_; ++i) {
    variant_outputs_.push_back(preview_pipeline_.CreateOutput());
  }

  initialized_ = true;
}

void ModelPreview::Render() {
  if (!initialized_)
    Init();

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
  ImGui::Begin("Procedural Preview", nullptr);

  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  RenderToolbar(draw_list);

  ImGui::BeginChild("GridArea", ImVec2(0, 0), false);
  RenderGrid(draw_list);
  ImGui::EndChild();

  ImGui::End();
  ImGui::PopStyleVar();

  // Render the preview pipeline framebuffers (Submits bgfx instructions)
  preview_pipeline_.Render();
}

void ModelPreview::RenderToolbar(ImDrawList* draw_list) {
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));

  if (ImGui::Button(ICON_FA_ROTATE " Generate")) {
    current_seed_++;
  }

  ImGui::SameLine();
  ImGui::SetNextItemWidth(150.0f);
  int seed_int = static_cast<int>(current_seed_);
  if (ImGui::InputInt("Seed", &seed_int)) {
    current_seed_ = static_cast<uint32_t>(seed_int);
  }

  ImGui::SameLine(ImGui::GetContentRegionAvail().x - 120.0f);
  ImGui::SetNextItemWidth(120.0f);
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

  if (ImGui::BeginTable("VariantGrid", columns)) {
    for (int i = 0; i < total_variants_; i++) {
      ImGui::TableNextColumn();

      ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
      ImVec2 rect_min = cursor_pos;
      ImVec2 rect_max =
          ImVec2(rect_min.x + thumbnail_size_, rect_min.y + thumbnail_size_);

      ImGui::InvisibleButton(("##var_" + std::to_string(i)).c_str(),
                             ImVec2(thumbnail_size_, thumbnail_size_));
      bool hovered = ImGui::IsItemHovered();
      bool selected = (s_selected_variant_ == i);

      if (ImGui::IsItemClicked()) {
        s_selected_variant_ = i;
      }

      ImU32 bg_color = EditorColor::element;  // #212121
      if (hovered)
        bg_color = EditorColor::element_hovered;
      if (selected)
        bg_color = EditorColor::element_active;  // #4D5C74 Slate Blue

      // Draw standard card background
      draw_list->AddRectFilled(rect_min, rect_max, bg_color, 4.0f);

      // Add Pipeline Render Instruction for this variant
      size_t out_idx = variant_outputs_[i];
      preview_pipeline_.ResizeOutput(out_idx, thumbnail_size_ * dpi,
                                     thumbnail_size_ * dpi);

      PreviewRenderInstruction inst;
      inst.output_index = out_idx;
      inst.background_color = glm::vec4(
          0.0f, 0.0f, 0.0f, 0.0f);  // Transparent so UI background shows

      // TODO: inst.model = procedurally_generated_models[i];
      inst.model_transform.scale_ = glm::vec3(1.0f);
      inst.camera_transform.position_ = glm::vec3(0.0f, 0.0f, -5.0f);

      preview_pipeline_.AddRenderInstruction(inst);

      // Draw Framebuffer Texture
      bgfx::TextureHandle tex = preview_pipeline_.GetOutputTexture(out_idx);
      if (bgfx::isValid(tex)) {
        // BGFX ImGui texture cast
        ImGui::SetCursorScreenPos(rect_min);
        ImGui::Image((ImTextureID)(uintptr_t)tex.idx,
                     ImVec2(thumbnail_size_, thumbnail_size_));
      } else {
        // Fallback Placeholder Icon if texture isn't ready or model is null
        ImGui::PushFont(EditorStyles::GetFonts().h2);
        std::string icon = ICON_FA_DRAGON;
        ImVec2 text_size = ImGui::CalcTextSize(icon.c_str());
        draw_list->AddText(
            ImVec2(rect_min.x + (thumbnail_size_ - text_size.x) * 0.5f,
                   rect_min.y + (thumbnail_size_ - text_size.y) * 0.5f),
            EditorColor::text, icon.c_str());
        ImGui::PopFont();
      }

      // Border and Label
      draw_list->AddRect(rect_min, rect_max, EditorColor::border_color, 4.0f, 0,
                         1.0f);

      std::string label = "Var " + std::to_string(i);
      draw_list->AddText(ImVec2(rect_min.x + 8.0f, rect_max.y - 20.0f),
                         EditorColor::text_transparent, label.c_str());
    }
    ImGui::EndTable();
  }
}