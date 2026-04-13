#include "model_viewer.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "editor/gui/styles/editor_styles.h"
#include "engine/ecs/components/transform_component.h"
#include "engine/renderer/model/model.h"

ModelViewer::ModelViewer(PreviewData* data) : data_(data) {}

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
  ImGui::Begin(
      "Model View", nullptr,
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

  RenderViewport();

  ImGui::Dummy(ImVec2(0, 8.0f));
  // Wrap the properties in a scrollable child region
  ImGui::BeginChild("PropertiesArea", ImVec2(0, 0), false,
                    ImGuiWindowFlags_None);
  RenderModelProperties();
  ImGui::EndChild();
  ImGui::End();
  ImGui::PopStyleVar();

  // Submit instructions to bgfx
  preview_pipeline_.Render();
}

void ModelViewer::RenderViewport() {
  int current_instance = data_->selected_instance;
  bool has_instance =
      data_->model && current_instance >= 0 &&
      current_instance < static_cast<int>(data_->instances.size());

  ImGui::TextDisabled("Instance %d / %d", current_instance,
                      static_cast<int>(data_->instances.size()));

  ImVec2 avail = ImGui::GetContentRegionAvail();
  float viewport_height = avail.y * 0.60f;
  float dpi = WindowManager::GetDPIScale();

  ImGui::BeginChild(
      "ModelViewport", ImVec2(0, viewport_height), true,
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

  ImVec2 p_min = ImGui::GetWindowPos();
  ImVec2 p_max = ImVec2(p_min.x + ImGui::GetWindowWidth(),
                        p_min.y + ImGui::GetWindowHeight());
  ImVec2 size = ImGui::GetContentRegionAvail();

  preview_pipeline_.ResizeOutput(viewport_output_, size.x * dpi, size.y * dpi);

  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRectFilled(p_min, p_max, EditorColor::background);
  //
  // BOUNDING BOX SCALING
  //
  glm::vec3 center = glm::vec3(0.0f);
  float max_dim = 1.0f;
  if (data_->model) {
    const auto& metrics = data_->model->GetMetrics();
    center = (metrics.min_point + metrics.max_point) * 0.5f;
    glm::vec3 dims = metrics.max_point - metrics.min_point;
    max_dim = glm::max(dims.x, glm::max(dims.y, dims.z));
    if (max_dim < 0.001f)
      max_dim = 1.0f;
  }

  // Scales the model down
  float norm_scale = 1.0f / max_dim;

  // Reset smoothly when a new variant is clicked
  static int last_instance = -1;
  if (current_instance != last_instance && data_->model) {
    camera_distance_ = 2.5f;
    model_pitch_ = -0.2f;
    model_yaw_ = 0.0f;
    camera_pan_x_ = 0.0f;
    camera_pan_y_ = 0.0f;
    last_instance = current_instance;
  }

  //
  // CAMERA CONTROLS
  //
  ImGui::InvisibleButton("##ModelInteract", size);
  bool is_hovered = ImGui::IsItemHovered();

  if (is_hovered) {
    float scroll = ImGui::GetIO().MouseWheel;
    if (scroll != 0.0f) {
      camera_distance_ -= scroll * (camera_distance_ * 0.1f);
      camera_distance_ = glm::clamp(camera_distance_, 0.01f, 50.0f);
      ImGui::GetIO().MouseWheel = 0.0f;  // Eat scroll input
    }
    float wasd_speed = camera_distance_ * 0.015f;
    if (ImGui::IsKeyDown(ImGuiKey_W))
      camera_pan_y_ -= wasd_speed;
    if (ImGui::IsKeyDown(ImGuiKey_S))
      camera_pan_y_ += wasd_speed;
    if (ImGui::IsKeyDown(ImGuiKey_A))
      camera_pan_x_ -= wasd_speed;
    if (ImGui::IsKeyDown(ImGuiKey_D))
      camera_pan_x_ += wasd_speed;
  }

  //
  // TRACKPAD
  //
  static bool is_alt_dragging = false;
  if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
      ImGui::GetIO().KeyAlt) {
    is_alt_dragging = true;
  }
  if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    is_alt_dragging = false;
  }

  if (is_alt_dragging) {
    ImVec2 delta = ImGui::GetIO().MouseDelta;
    if (ImGui::GetIO().KeyShift) {
      // ALT + SHIFT + LMB = PAN
      camera_pan_x_ += delta.x * (camera_distance_ * 0.001f);
      camera_pan_y_ -= delta.y * (camera_distance_ * 0.001f);
    } else {
      // ALT + LMB = ORBIT
      model_yaw_ += delta.x * 0.01f;
      model_pitch_ += delta.y * 0.01f;
      model_pitch_ = glm::clamp(model_pitch_, -1.5f, 1.5f);
    }
  }

  static bool is_dragging = false;
  if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
    is_dragging = true;
  }
  if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
    is_dragging = false;
  }

  if (is_dragging) {
    ImVec2 delta = ImGui::GetIO().MouseDelta;
    if (ImGui::GetIO().KeyShift) {
      // SHIFT + MMB = PAN (Moves focal point relative to camera distance)
      camera_pan_x_ -= delta.x * (camera_distance_ * 0.001f);
      camera_pan_y_ -= delta.y * (camera_distance_ * 0.001f);
    } else {
      // MMB = TURNTABLE ORBIT
      model_yaw_ -= delta.x * 0.01f;
      model_pitch_ -= delta.y * 0.01f;
      model_pitch_ = glm::clamp(model_pitch_, -1.5f, 1.5f);
    }
  }

  // Calculate Camera Rotation
  glm::quat q_pitch = glm::angleAxis(model_pitch_, glm::vec3(1.0f, 0.0f, 0.0f));
  glm::quat q_yaw = glm::angleAxis(model_yaw_, glm::vec3(0.0f, 1.0f, 0.0f));
  glm::quat q_cam = q_yaw * q_pitch;

  // Calculate panning offsets relative to the camera's view
  glm::vec3 right = q_cam * glm::vec3(1.0f, 0.0f, 0.0f);
  glm::vec3 up = q_cam * glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 pan_offset = (right * camera_pan_x_) + (up * camera_pan_y_);

  // Apply a 180-degree rotation on X to correct upside-down models
  glm::quat base_rotation =
      glm::angleAxis(glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));

  // The final focal point is the rotated center of the scaled model + user pan
  // offsets
  glm::vec3 rotated_center = base_rotation * (center * norm_scale);
  glm::vec3 focal_point = (center * norm_scale) + pan_offset;

  if (has_instance) {
    PreviewRenderInstruction inst;
    inst.output_index = viewport_output_;
    inst.background_color = glm::vec4(0.094f, 0.094f, 0.094f, 1.0f);
    inst.model = data_->model.get();

    // Filter to only selected instance's meshes
    const auto& resolved = data_->instances[current_instance];
    for (const auto& desc : resolved.descriptors) {
      for (int idx : desc.mesh_indices) {
        inst.mesh_filter.push_back(static_cast<uint32_t>(idx));
      }
    }

    // Keep model at origin to prevent lasso clipping
    inst.model_transform.position_ = glm::vec3(0.0f);
    inst.model_transform.rotation_ = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    inst.model_transform.scale_ = glm::vec3(norm_scale);

    // Orbit camera perfectly around the focal point
    inst.camera_transform.rotation_ = q_cam;
    inst.camera_transform.position_ =
        focal_point + (q_cam * glm::vec3(0.0f, 0.0f, -camera_distance_));

    preview_pipeline_.AddRenderInstruction(inst);
  }

  bgfx::TextureHandle tex =
      preview_pipeline_.GetOutputTexture(viewport_output_);
  if (has_instance && bgfx::isValid(tex)) {
    ImGui::SetCursorScreenPos(p_min);
    ImGui::Image((ImTextureID)(uintptr_t)tex.idx, size, ImVec2(0, 1),
                 ImVec2(1, 0));
  } else {
    ImGui::PushFont(EditorStyles::GetFonts().h1);
    std::string icon = ICON_FA_CUBE;
    ImVec2 text_size = ImGui::CalcTextSize(icon.c_str());
    draw_list->AddText(ImVec2(p_min.x + (size.x - text_size.x) * 0.5f,
                              p_min.y + (size.y - text_size.y) * 0.5f),
                       EditorColor::selection, icon.c_str());
    ImGui::PopFont();
  }

  ImGui::EndChild();
}

void ModelViewer::RenderModelProperties() {
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));

  if (ImGui::CollapsingHeader("Mesh Data", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Indent(10.0f);
    ImGui::Dummy(ImVec2(0, 4.0f));

    if (data_->model) {
      const auto& metrics = data_->model->GetMetrics();
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Topology");
      ImGui::Text("Vertices: %u", metrics.n_vertices);
      ImGui::Text("Triangles: %u", metrics.n_faces);

      ImGui::Dummy(ImVec2(0, 4.0f));
      ImGui::Separator();
      ImGui::Dummy(ImVec2(0, 4.0f));

      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Materials");
      ImGui::Text("Slots: %u", metrics.n_materials);
    } else {
      ImGui::TextDisabled("No model loaded");
    }

    ImGui::Dummy(ImVec2(0, 4.0f));
    ImGui::Unindent(10.0f);
  }

  ImGui::PopStyleVar();
}

void ModelViewer::RenderInstanceList() {
  int idx = data_->selected_instance;

  if (idx < 0 || idx >= static_cast<int>(data_->instances.size()))
    return;

  const auto& resolved = data_->instances[idx];

  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));

  // Display selection path of current instance
  if (ImGui::CollapsingHeader("Instance Parts",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Indent(10.0f);
    ImGui::Dummy(ImVec2(0, 4.0f));

    ImGui::Text("Seed: %llu", resolved.seed);
    ImGui::Dummy(ImVec2(0, 4.0f));

    for (const auto& desc : resolved.descriptors) {
      ImGui::Text("%s", desc.descriptor_id.c_str());
    }

    ImGui::Dummy(ImVec2(0, 4.0f));
    ImGui::Unindent(10.0f);
  }

  ImGui::PopStyleVar();
}
