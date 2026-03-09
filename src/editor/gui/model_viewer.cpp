#include "model_viewer.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "editor/gui/styles/editor_styles.h"
#include "engine/ecs/components/transform_component.h"
#include "model_preview.h"

ModelViewer::ModelViewer() {}

ModelViewer::~ModelViewer() {
  if (initialized_) {
    preview_pipeline_.Destroy();
  }
}

void ModelViewer::Init() {
  if (initialized_)
    return;

  preview_pipeline_.Create();
  viewport_output_ = preview_pipeline_.CreateOutput();

  initialized_ = true;
}

void ModelViewer::Render() {
  if (!initialized_)
    Init();

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
  ImGui::Begin("Model View", nullptr);

  RenderViewport();

  ImGui::Dummy(ImVec2(0, 8.0f));

  RenderModelProperties();

  ImGui::End();
  ImGui::PopStyleVar();

  // Submit instructions to bgfx
  preview_pipeline_.Render();
}

void ModelViewer::RenderViewport() {
  int current_variant = ModelPreview::GetSelectedVariant();

  std::string model_path = "MODELS/PLANETS/PROCEDURAL/VARIANT_" +
                           std::to_string(current_variant) + ".SCENE.MBIN";
  ImGui::TextDisabled("%s", model_path.c_str());

  ImVec2 avail = ImGui::GetContentRegionAvail();
  float viewport_height = avail.y * 0.60f;
  float dpi = WindowManager::GetDPIScale();

  ImGui::BeginChild("ModelViewport", ImVec2(0, viewport_height), true,
                    ImGuiWindowFlags_NoScrollbar);

  ImVec2 p_min = ImGui::GetWindowPos();
  ImVec2 p_max = ImVec2(p_min.x + ImGui::GetWindowWidth(),
                        p_min.y + ImGui::GetWindowHeight());
  ImVec2 size = ImGui::GetContentRegionAvail();

  // Update pipeline size
  preview_pipeline_.ResizeOutput(viewport_output_, size.x * dpi, size.y * dpi);

  // Background color (#181818 to match panels)
  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRectFilled(p_min, p_max, EditorColor::background);

  // Interaction (Spin Model)
  ImGui::InvisibleButton("##ModelInteract", size);
  if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    model_rotation_x_ += ImGui::GetIO().MouseDelta.y * 0.01f;
    model_rotation_y_ += ImGui::GetIO().MouseDelta.x * 0.01f;
  }
  if (ImGui::IsItemHovered()) {
    camera_distance_ -= ImGui::GetIO().MouseWheel * 0.5f;
    camera_distance_ = glm::clamp(camera_distance_, 1.0f, 20.0f);
  }

  // Submit Render Instruction
  PreviewRenderInstruction inst;
  inst.output_index = viewport_output_;
  inst.background_color =
      glm::vec4(0.094f, 0.094f, 0.094f, 1.0f);  // Match #181818

  // TODO: inst.model = procedurally_generated_models[current_variant];
  inst.model_transform.scale_ = glm::vec3(1.0f);

  glm::quat qx = glm::angleAxis(model_rotation_x_, glm::vec3(1.0f, 0.0f, 0.0f));
  glm::quat qy = glm::angleAxis(model_rotation_y_, glm::vec3(0.0f, 1.0f, 0.0f));
  inst.model_transform.rotation_ = qy * qx;

  inst.camera_transform.position_ = glm::vec3(0.0f, 0.0f, -camera_distance_);

  preview_pipeline_.AddRenderInstruction(inst);

  // Draw 3D Viewport Framebuffer Texture
  bgfx::TextureHandle tex =
      preview_pipeline_.GetOutputTexture(viewport_output_);
  if (bgfx::isValid(tex)) {
    ImGui::SetCursorScreenPos(p_min);
    ImGui::Image((ImTextureID)(uintptr_t)tex.idx, size);
  }

  ImGui::EndChild();
}

void ModelViewer::RenderModelProperties() {
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));

  if (ImGui::CollapsingHeader("Procedural Mesh Data",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Indent(10.0f);
    ImGui::Dummy(ImVec2(0, 4.0f));

    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Topology");
    ImGui::Text("Vertices: %d", 14502);
    ImGui::Text("Triangles: %d", 28900);

    ImGui::Dummy(ImVec2(0, 4.0f));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 4.0f));

    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Materials");
    ImGui::Text("Slots Active: %d", 2);
    ImGui::Text("Base Material: %s", "Mat_Default_01");

    ImGui::Dummy(ImVec2(0, 4.0f));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 4.0f));

    ImGui::AlignTextToFramePadding();
    ImGui::Text("LOD Bias");
    ImGui::SameLine(100.0f);
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::SliderFloat("##LOD", &lod_bias_, 0.1f, 5.0f, "%.2f");

    ImGui::Dummy(ImVec2(0, 4.0f));
    ImGui::Unindent(10.0f);
  }

  ImGui::PopStyleVar();
}