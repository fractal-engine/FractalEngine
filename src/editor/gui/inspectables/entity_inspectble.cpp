#include "entity_inspectable.h"

#include "editor/registry/component_registry.h"

#include <imgui.h>

//---------------------------------------------------------------------------
// Dynamic inspector for entites
// TODO:
// - Replace placeholder imgui with reusable UI helper functions (im_components)
// - Add ecs/components files dependency
// - Implement HierarchyItem wrapper for entity handles
//---------------------------------------------------------------------------

EntityInspectable::EntityInspectable(Entity entity) : entity_(entity) {}

void EntityInspectable::RenderStaticContent(ImDrawList& draw_list) {
  auto& ecs = ECS::Main();

  if (!ecs.Reg().valid(entity_)) {
    ImGui::TextDisabled("Invalid entity");
    return;
  }

  // Get entity name from TransformComponent
  std::string name = "Entity";
  if (ecs.Has<TransformComponent>(entity_)) {
    auto& transform = ecs.Get<TransformComponent>(entity_);
    if (!transform.name_.empty()) {
      name = transform.name_;
    }
  }

  // Header with entity name
  ImGui::Text("%s", name.c_str());
  ImGui::SameLine();
  ImGui::TextDisabled("(ID: %u)", entt::to_integral(entity_));

  ImGui::Spacing();

  // Add Component button
  if (ImGui::Button("Add Component")) {
    ImGui::OpenPopup("AddComponentPopup");
  }

  // Component selection popup
  if (ImGui::BeginPopup("AddComponentPopup")) {
    ImGui::TextDisabled("Available Components:");
    ImGui::Separator();

    // TODO: Populate from ComponentRegistry::GetAddableComponents()
    if (ImGui::Selectable("Volume")) {
      // TODO: Add VolumeComponent to entity
    }

    ImGui::EndPopup();
  }

  ImGui::Separator();
}

void EntityInspectable::RenderDynamicContent(ImDrawList& draw_list) {
  // Iterate registered components and draw their inspectors
  const auto& registry = ComponentRegistry::Get();
  const auto& keys_ordered = ComponentRegistry::GetKeysOrdered();

  for (const auto& key : keys_ordered) {
    auto it = registry.find(key);
    if (it != registry.end()) {
      it->second.TryDrawInspectable(entity_);
    }
  }
}