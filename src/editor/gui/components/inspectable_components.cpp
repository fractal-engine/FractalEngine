#include "inspectable_components.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#include <cstdint>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <unordered_map>

#include "editor/editor_ui.h"
#include "editor/vendor/IconFontCppHeaders/IconsFontAwesome6.h"
#include "engine/ecs/world.h"
#include "engine/transform/transform.h"

// TODO: #include "editor/ui/components/im_components.h"
// TODO: #include "rendering/icons/icon_pool.h"

namespace InspectableComponents {

//=============================================================================
// INTERNAL STATE
//=============================================================================

// Store collapsed state for each component
std::unordered_map<uint32_t, bool> g_opened;

// Return pointer to opened state
bool* _IsOpened(uint32_t component_id) {
  return &g_opened[component_id];
}

// Remove ECS component from entity
template <typename T>
void _EcsRemove(Entity entity) {
  ECS::Main().Remove<T>(entity);
}

//=============================================================================
// COMPONENT HEADER DRAWING
//=============================================================================

/**
 *
 * Draw a component header, return true if expanded
 * Set enabledPtr to nullptr if component can't be disabled
 * Set removedPtr to nullptr if component can't be removed
 * Set alwaysOpened to true for components that can't be collapsed
 *
 */
bool _BeginComponent(const std::string& identifier,
                     uint32_t icon,  // TODO: Use IconPool texture ID
                     bool* enabled_ptr = nullptr, bool* removed_ptr = nullptr,
                     bool always_opened = false) {
  ImDrawList& draw_list = *ImGui::GetWindowDrawList();

  ImVec2 content_avail = ImGui::GetContentRegionAvail();
  ImVec2 cursor_pos = ImGui::GetCursorScreenPos();

  // TODO: Use EditorSizing constants
  float title_height = 16.0f;
  ImVec2 title_padding = ImVec2(10.0f, 10.0f);

  float height = title_height + title_padding.y * 2;
  float y_margin = 2.0f;
  ImVec2 size = ImVec2(content_avail.x, height);

  ImVec2 p0 = ImVec2(cursor_pos.x, cursor_pos.y + y_margin);
  ImVec2 p1 = ImVec2(p0.x + size.x, p0.y + size.y);
  const bool hovered = ImGui::IsMouseHoveringRect(p0, p1);

  ImVec2 cursor = p0 + title_padding;

  bool always_enabled = !enabled_ptr;

  uint32_t component_id = entt::hashed_string::value(identifier.c_str());
  bool* opened_ptr = _IsOpened(component_id);

  // Click to collapse
  if (!always_opened) {
    if (hovered && ImGui::IsMouseClicked(2))
      *opened_ptr = !(*opened_ptr);
  }

  // Draw background
  draw_list.AddRectFilled(p0, p1, IM_COL32(30, 30, 30, 255), 10.0f);

  // Draw expansion caret
  if (always_opened) {
    draw_list.AddText(cursor, IM_COL32(255, 255, 255, 255), ICON_FA_CARET_DOWN);
  } else {
    const char* caret = *opened_ptr ? ICON_FA_CARET_DOWN : ICON_FA_CARET_RIGHT;
    draw_list.AddText(cursor, IM_COL32(200, 200, 200, 255), caret);
  }
  cursor.x += 22.0f;

  // Draw component icon
  // TODO: Use IconPool::Get(icon) texture
  // ImVec2 icon_size = ImVec2(18.0f, 18.0f);
  // draw_list.AddImage(icon, cursor, cursor + icon_size, ImVec2(0, 1),
  // ImVec2(1, 0)); cursor.x += icon_size.x + 8.0f;
  cursor.x += 8.0f;  // ! Placeholder spacing

  // Draw enabled checkbox
  if (!always_enabled) {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 3));

    ImVec2 internal_cursor = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(cursor + ImVec2(0.0f, -2.0f));
    std::string checkbox_id = "##enabled_" + identifier;
    ImGui::Checkbox(checkbox_id.c_str(), enabled_ptr);
    ImGui::SetCursorScreenPos(internal_cursor);
    cursor.x += 28.0f;

    ImGui::PopStyleVar();
  }

  // Draw component name
  draw_list.AddText(cursor, IM_COL32(255, 255, 255, 255), identifier.c_str());

  // Draw remove button
  if (removed_ptr) {
    float button_size = 20.0f;
    ImVec2 button_pos =
        ImVec2(p1.x - button_size - title_padding.x, cursor.y - 2.0f);

    ImGui::SetCursorScreenPos(button_pos);
    std::string remove_id = "##remove_" + identifier;
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 65, 65, 90));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 65, 65, 150));
    if (ImGui::Button((std::string(ICON_FA_XMARK) + remove_id).c_str(),
                      ImVec2(button_size, button_size))) {
      *removed_ptr = true;
    }
    ImGui::PopStyleColor(3);
  }

  // Advance cursor for content
  bool currently_opened = always_opened ? true : *opened_ptr;

  if (currently_opened) {
    ImGui::Dummy(ImVec2(0.0f, size.y + y_margin + 12.0f));
  } else {
    ImGui::Dummy(ImVec2(size.x, size.y + y_margin));
  }

  return currently_opened;
}

void _EndComponent() {
  ImGui::Dummy(ImVec2(0.0f, 20.0f));
}

//=============================================================================
// HELPER WIDGETS
//=============================================================================
void _Headline(const std::string& label) {
  // TODO: Use IMComponents::label with bold font
  ImGui::TextUnformatted(label.c_str());
  ImGui::Dummy(ImVec2(0.0f, 1.0f));
}

void _SpacingS() {
  ImGui::Dummy(ImVec2(0.0f, 3.0f));
}

void _SpacingM() {
  ImGui::Dummy(ImVec2(0.0f, 10.0f));
}

// TODO: Replace with IMComponents::input variants
void _InputFloat3(const char* label, glm::vec3& value) {
  ImGui::DragFloat3(label, glm::value_ptr(value), 0.1f);
}

void _InputFloat(const char* label, float& value) {
  ImGui::DragFloat(label, &value, 0.1f);
}

void _InputInt(const char* label, int& value) {
  ImGui::DragInt(label, &value);
}

void _InputBool(const char* label, bool& value) {
  ImGui::Checkbox(label, &value);
}

void _InputColor3(const char* label, glm::vec3& value) {
  ImGui::ColorEdit3(label, glm::value_ptr(value));
}

void _Label(const std::string& text) {
  ImGui::TextUnformatted(text.c_str());
}

void _VectorLabel(const std::string& label, const glm::vec3& value) {
  ImGui::Text("%s: (%.2f, %.2f, %.2f)", label.c_str(), value.x, value.y,
              value.z);
}

//=============================================================================
// COMPONENT INSPECTABLES
//=============================================================================
void DrawTransform(Entity entity, TransformComponent& transform) {
  if (_BeginComponent("Transform", 0, nullptr, nullptr, true)) {
    _Headline("Properties");

    // TODO: Use IMComponents::input
    _InputFloat3("Position", transform.local_position_);

    glm::vec3 euler = glm::degrees(transform.euler_angles_);
    if (ImGui::DragFloat3("Rotation", glm::value_ptr(euler), 1.0f)) {
      transform.euler_angles_ = glm::radians(euler);
      Transform::SetEulerAngles(transform, transform.euler_angles_,
                                Space::LOCAL);
    }

    _InputFloat3("Scale", transform.local_scale_);

    if (Transform::HasParent(transform)) {
      auto& parent = Transform::GetParent(transform);
      _Label("Parent: " + parent.name_);
    }

    _Label("ID: " + std::to_string(transform.id_));
    _Label("Depth: " + std::to_string(transform.depth_));

    if (Transform::HasParent(transform)) {
      _VectorLabel("World Position",
                   Transform::GetPosition(transform, Space::WORLD));
      _VectorLabel("World Scale", Transform::GetScale(transform, Space::WORLD));
    }

    // Apply changes
    Transform::SetPosition(transform, transform.local_position_, Space::LOCAL);
    Transform::SetScale(transform, transform.local_scale_, Space::LOCAL);

    _EndComponent();
  }
}

void DrawMeshRenderer(Entity entity, MeshRendererComponent& renderer) {
  bool removed = false;

  if (_BeginComponent("Mesh Renderer", 0, &renderer.visible_, &removed)) {
    _Headline("General");

    if (renderer.mesh_) {
      _Label("Index Count: " + std::to_string(renderer.mesh_->IndexCount()));
    } else {
      ImGui::TextDisabled("No mesh assigned");
    }

    // TODO: Use Material display instead ?
    // if (renderer.material_) {
    //   _Label("Material ID: " + std::to_string(renderer.material_->GetId()));
    // }

    _InputBool("Cast Shadows", renderer.cast_shadows_);

    _EndComponent();
  }

  if (removed)
    _EcsRemove<MeshRendererComponent>(entity);
}

void DrawCamera(Entity entity, CameraComponent& camera) {
  bool removed = false;

  if (_BeginComponent("Camera", 0, &camera.enabled, &removed)) {
    _Headline("General");

    _InputFloat("FOV", camera.fov);
    _InputFloat("Near", camera.near_plane);
    _InputFloat("Far", camera.far_plane);

    _EndComponent();
  }

  if (removed)
    _EcsRemove<CameraComponent>(entity);
}

void DrawDirectionalLightComponent(Entity entity,
                                   DirectionalLightComponent& light) {
  bool removed = false;

  if (_BeginComponent("Directional Light", 0, &light.enabled, &removed)) {
    _Headline("Properties");

    _InputFloat("Intensity", light.intensity);
    _InputColor3("Color", light.color);

    _SpacingS();
    _Headline("Shadows");

    // TODO: Shadow settings
    bool tmp_cast = true;
    bool tmp_soft = true;
    _InputBool("Cast Shadows", tmp_cast);
    _InputBool("Soft Shadows", tmp_soft);

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 135));

    // TODO: update message here
    ImGui::TextWrapped(
        ICON_FA_TRIANGLE_EXCLAMATION
        " In-editor shadows aren't dynamic yet and can't be changed currently");
    ImGui::PopStyleColor();

    _EndComponent();
  }

  if (removed)
    _EcsRemove<DirectionalLightComponent>(entity);
}

void DrawPointLightComponent(Entity entity, PointLightComponent& light) {
  bool removed = false;

  if (_BeginComponent("Point Light", 0, &light.enabled, &removed)) {
    _Headline("Properties");

    _InputFloat("Intensity", light.intensity);
    _InputColor3("Color", light.color);
    _InputFloat("Range", light.range);
    _InputFloat("Falloff", light.falloff);

    _SpacingS();
    _Headline("Shadows");

    bool tmp_cast = false;
    bool tmp_soft = false;
    _InputBool("Cast Shadows", tmp_cast);
    _InputBool("Soft Shadows", tmp_soft);

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 135));

    // TODO: update message here
    ImGui::TextWrapped(
        ICON_FA_TRIANGLE_EXCLAMATION
        " In-editor shadows aren't dynamic yet and can't be changed currently");
    ImGui::PopStyleColor();

    _EndComponent();
  }

  if (removed)
    _EcsRemove<PointLightComponent>(entity);
}

void DrawSpotlightComponent(Entity entity, SpotlightComponent& spotlight) {
  bool removed = false;

  if (_BeginComponent("Spotlight", 0, &spotlight.enabled, &removed)) {
    _Headline("Properties");

    _InputFloat("Intensity", spotlight.intensity);
    _InputColor3("Color", spotlight.color);
    _InputFloat("Range", spotlight.range);
    _InputFloat("Falloff", spotlight.falloff);
    _InputFloat("Inner Angle", spotlight.inner_angle);
    _InputFloat("Outer Angle", spotlight.outer_angle);

    _SpacingS();
    _Headline("Shadows");

    bool tmp_cast = true;
    bool tmp_soft = true;
    _InputBool("Cast Shadows", tmp_cast);
    _InputBool("Soft Shadows", tmp_soft);

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 135));

    // TODO: update message here
    ImGui::TextWrapped(
        ICON_FA_TRIANGLE_EXCLAMATION
        " In-editor shadows aren't dynamic yet and can't be changed currently");
    ImGui::PopStyleColor();

    _EndComponent();
  }

  if (removed)
    _EcsRemove<SpotlightComponent>(entity);
}

// TODO: component inspectables
// void DrawVelocityBlur(Entity entity, VelocityBlurComponent& velocity);
// void DrawBoxCollider(Entity entity, BoxColliderComponent& collider);
// void DrawSphereCollider(Entity entity, SphereColliderComponent& collider);
// void DrawRigidbody(Entity entity, RigidbodyComponent& rigidbody);
// void DrawAudioListener(Entity entity, AudioListenerComponent& listener);
// void DrawAudioSource(Entity entity, AudioSourceComponent& source);

// TODO: Post-processing inspectables
// void DrawColor(PostProcessing::Color& color);
// void DrawMotionBlur(PostProcessing::MotionBlur& motionBlur);
// void DrawBloom(PostProcessing::Bloom& bloom);
// void DrawChromaticAberration(PostProcessing::ChromaticAberration& ca);
// void DrawVignette(PostProcessing::Vignette& vignette);
// void DrawAmbientOcclusion(PostProcessing::AmbientOcclusion& ao);

}  // namespace InspectableComponents