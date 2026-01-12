#ifndef CAMERA_CONTROLS_H
#define CAMERA_CONTROLS_H

#include <imgui.h>
#include "editor/runtime/runtime.h"
#include "engine/ecs/world.h"

namespace Panels {

inline void CameraControls() {
  ImGui::BeginChild("CameraControls", ImVec2(300, 0), true);
  ImGui::Text("Camera Controls");
  ImGui::Separator();

  // Get selected camera entity from hierarchy
  Entity selected = EditorUI::Get()->GetSelectedEntity();
  auto& world = ECS::Main();

  if (selected != entt::null && world.Has<CameraComponent>(selected)) {
    auto& camera = world.Get<CameraComponent>(selected);
    auto& transform = world.Get<TransformComponent>(selected);

    // Camera properties
    if (ImGui::SliderFloat("FOV", &camera.fov_, 30.0f, 120.0f))
      transform.modified_ = true;

    if (ImGui::SliderFloat("Near", &camera.near_plane_, 0.1f, 10.0f))
      transform.modified_ = true;

    if (ImGui::SliderFloat("Far", &camera.far_plane_, 100.0f, 10000.0f))
      transform.modified_ = true;

    ImGui::Checkbox("Enabled", &camera.enabled_);

    if (ImGui::Button("Reset Camera")) {
      camera.fov_ = 60.0f;
      camera.near_plane_ = 0.1f;
      camera.far_plane_ = 1000.0f;
    }
  } else {
    ImGui::TextDisabled("Select a camera entity to edit");
  }

  ImGui::EndChild();
}

}  // namespace Panels

#endif  // CAMERA_CONTROLS_H
