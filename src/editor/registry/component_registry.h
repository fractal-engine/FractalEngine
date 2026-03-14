#ifndef COMPONENT_REGISTRY_H
#define COMPONENT_REGISTRY_H

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/ecs/ecs_collection.h"
#include "engine/renderer/gizmos/imgizmo.h"

struct ComponentInfo {

  // Component name
  std::string name;

  // Icon identifier
  std::string icon;

  // Component description
  std::string description;

  // Return true if entity has a component
  std::function<bool(Entity)> has;

  // Add component to entity
  std::function<void(Entity)> add;

  // Draw inpsector UI for component if entity has it
  std::function<void(Entity)> try_draw_inspectable;

  // Draw gizmo for component if entity has it
  std::function<void(Entity, IMGizmo&)> try_draw_gizmo;

  // Draw scene view icons for all instances of a component
  std::function<void(IMGizmo&, TransformComponent&)> try_draw_scene_icons;
};

namespace ComponentRegistry {

// Initialize component registry
void Create();

// Get registered components
const std::unordered_map<std::string, ComponentInfo>& Get();

// Get ordered keys
const std::vector<std::string>& OrderedKeys();

}  // namespace ComponentRegistry

#endif  // COMPONENT_REGISTRY_H