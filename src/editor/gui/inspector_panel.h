#ifndef INSPECTOR_PANEL_H
#define INSPECTOR_PANEL_H

#include "engine/ecs/components/transform_component.h"
#include "engine/ecs/components/volume_component.h"

#include "engine/pcg/generator_base.h"
#include "engine/pcg/generator_resource.h"
#include "engine/pcg/pcg_engine.h"

#include "engine/context/engine_context.h"
#include "engine/ecs/world.h"

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

inline void InspectVolume(Entity entity) {
  auto& world = ECS::Main();

  if (!world.Has<VolumeComponent>(entity))
    return;

  auto& volume = world.Get<VolumeComponent>(entity);

  if (!ImGui::CollapsingHeader("Volume", ImGuiTreeNodeFlags_DefaultOpen))
    return;

  bool changed = false;

  // Resolution
  int res = volume.resolution;
  if (ImGui::SliderInt("Resolution", &res, 64, 4096)) {
    volume.resolution = static_cast<uint16_t>(res);
    changed = true;
  }

  // Generator selection (resource picker)
  // TODO: Resource picker widget
  ImGui::Text("Generator: %u", volume.generator_id);

  // If generator is Graph type, show edit button
  if (volume.generator_id != 0) {
    auto generator_res =
        EngineContext::resourceManager().GetResourceAs<PCG::GeneratorResource>(
            volume.generator_id);

    if (generator_res) {
      PCG::GeneratorBase* generator = generator_res->Get();
      if (generator && generator->GetType() == PCG::GeneratorType::Graph) {
        PCGEngine& pcg = EngineContext::Generator();

        if (pcg.active_graph == generator->GetGraph()) {
          if (ImGui::Button("Close Graph Editor")) {
            pcg.CloseGraphEditor();
          }
        } else {
          if (ImGui::Button("Edit Graph")) {
            pcg.SetActiveGraph(generator->GetGraph());
          }
        }
      }
    }
  }

  if (changed) {
    volume.dirty = true;
  }
}

}  // namespace Panels
#endif  // INSPECTOR_PANEL_H