#ifndef INSPECTOR_PANEL_H
#define INSPECTOR_PANEL_H

#include "engine/ecs/components/transform.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>  // Required for glm::value_ptr

namespace Panels {

// CHANGE THE FUNCTION SIGNATURE
// It no longer takes a vector of demo transforms. It takes a direct reference
// to the live TransformComponent of the selected entity.
inline void Inspector(TransformComponent& transform) {

  // The ImGui::Begin/End calls should be in editor_ui.cpp. This function
  // is only responsible for the *contents* of the panel.

  ImGui::Text("Entity Name: %s", transform.name_.c_str());
  ImGui::SameLine();
  // Display the entity's actual unique handle ID
  ImGui::TextDisabled("(ID: #%u)", entt::to_integral(transform.parent_));
  ImGui::Separator();
  ImGui::Spacing();

  if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {

    // Use a single flag to track if any value was modified this frame.
    bool changed = false;

    // BIND THE WIDGETS DIRECTLY TO THE COMPONENT'S DATA
    changed |= ImGui::DragFloat3("Position",
                                 glm::value_ptr(transform.position_), 0.1f);

    // Use the euler_angles_ member for the UI, which is more stable for
    // editing.
    if (ImGui::DragFloat3("Rotation", glm::value_ptr(transform.euler_angles_),
                          1.0f)) {
      // If the user drags the Euler angles, we update the official quaternion.
      transform.rotation_ = glm::quat(glm::radians(transform.euler_angles_));
      changed = true;
    }

    changed |=
        ImGui::DragFloat3("Scale", glm::value_ptr(transform.scale_), 0.05f);

    // IF ANY WIDGET WAS CHANGED, MARK THE COMPONENT AS DIRTY
    //    This tells the ECS to recalculate its matrices on the next frame.
    if (changed) {
      transform.modified_ = true;
    }
  }

  // TODO: Add inspectors for other components on the entity here.
  // For example: if (ecs.Has<MeshRendererComponent>(entity)) { ... }
}

}  // namespace Panels
#endif  // INSPECTOR_PANEL_H