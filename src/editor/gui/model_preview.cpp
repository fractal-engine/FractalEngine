#include "model_preview.h"

#include <algorithm>
#include "editor/gui/styles/editor_styles.h"
#include "engine/ecs/components/transform_component.h"  // For TransformComponent
#include "engine/renderer/model/model.h"

// Variables
int ModelPreview::s_selected_variant_ = 0;
float ModelPreview::s_model_scale_ = 1.0f;  // Default scale 1.0
std::shared_ptr<Model> ModelPreview::model_ = nullptr;

// Functions
int ModelPreview::GetSelectedVariant() {
  return s_selected_variant_;
}
std::shared_ptr<Model> ModelPreview::GetModel() {
  return model_;
}
float ModelPreview::GetModelScale() {
  return s_model_scale_;
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

  // ------------------------------------------------------------------------
  // [PCG INTEGRATION HOOK]
  // Currently, we load a hardcoded asset to visualize the UI pipeline.
  // How we handle the model structurally is identical to the final product.
  // Once the PCG framework is ready, this source will simply swap from
  // Model::Load() to the PCG generator output.
  // ------------------------------------------------------------------------
  model_ = Model::Load("/Users/louismercier/Projects/FractalEngine/build/macosx/x86_64/release/examples/example-project/loomis_head.glb");

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
  // Toolbar gets the parent window's draw list
  RenderToolbar(ImGui::GetWindowDrawList());

  ImGui::BeginChild("GridArea", ImVec2(0, 0), false);

  // Fetch the draw list INSIDE the child window so scrolling
  // automatically clips the images
  ImDrawList* child_draw_list = ImGui::GetWindowDrawList();
  RenderGrid(child_draw_list);
  ImGui::EndChild();

  ImGui::End();
  ImGui::PopStyleVar();

  // Render the preview pipeline framebuffers
  preview_pipeline_.Render();
}

void ModelPreview::RenderToolbar(ImDrawList* draw_list) {
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));

  if (ImGui::Button(ICON_FA_ROTATE " Generate")) {
    current_seed_++;
  }

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
      // Darker grey FBO background to contrast against the panel/highlight
      inst.background_color = glm::vec4(0.06f, 0.06f, 0.06f, 1.0f);

      inst.model = model_.get();
      inst.model_transform.scale_ = glm::vec3(s_model_scale_);
      inst.model_transform.rotation_ =
          glm::angleAxis(float(i) * 0.4f, glm::vec3(0.0f, 1.0f, 0.0f));
      inst.camera_transform.position_ = glm::vec3(0.0f, 0.0f, 6.0f);

      preview_pipeline_.AddRenderInstruction(inst);

      bgfx::TextureHandle tex = preview_pipeline_.GetOutputTexture(out_idx);

      if (model_ && bgfx::isValid(tex)) {
        // Draw the image slightly smaller than the cell so the
        // background 'bg_color' acts as a border frame
        float pad = 3.0f;
        ImVec2 img_min = ImVec2(rect_min.x + pad, rect_min.y + pad);
        ImVec2 img_max = ImVec2(rect_max.x - pad, rect_max.y - pad);

        draw_list->AddImage((ImTextureID)(uintptr_t)tex.idx, img_min, img_max,
                            ImVec2(0, 1), ImVec2(1, 0));
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

      // Pure white 'text' color draws cleanly over the
      // 3D render
      std::string label = "Var " + std::to_string(i);
      draw_list->AddText(ImVec2(rect_min.x + 8.0f, rect_max.y - 20.0f),
                         EditorColor::text, label.c_str());
    }
    ImGui::EndTable();
  }
}
