#include "transform_system.h"

#include <algorithm>

#include "engine/ecs/ecs_collection.h"
#include "engine/transform/transform.h"

// TODO: include diagnostics/profiler here

void TransformSystem::Perform(glm::mat4 viewProjection) {
  // Evaluate all transforms
  auto& ecs = ECS::Main();
  auto view = ecs.View<TransformComponent>();
  for (auto entity : view) {
    auto& transform = view.get<TransformComponent>(entity);
    if (Transform::IsRoot(transform)) {
      Evaluate(transform);
    }
    Transform::UpdateMVP(transform, viewProjection);
  }
}

void TransformSystem::Evaluate(TransformComponent& transform) {
  if (transform.modified_) {
    Transform::Evaluate(transform);
  }
  for (Entity child : transform.children_) {
    auto& ecs = ECS::Main();
    auto& childTransform = ecs.Get<TransformComponent>(child);
    Evaluate(childTransform, transform, transform.modified_);
  }
  transform.modified_ = false;
}

void TransformSystem::Evaluate(TransformComponent& transform,
                               const TransformComponent& parent,
                               bool propagateModified) {
  if (propagateModified)
    transform.modified_ = true;
  if (transform.modified_) {
    Transform::Evaluate(transform, parent);
  }
  for (Entity child : transform.children_) {
    auto& ecs = ECS::Main();
    auto& childTransform = ecs.Get<TransformComponent>(child);
    Evaluate(childTransform, transform, transform.modified_);
  }
  transform.modified_ = false;
}