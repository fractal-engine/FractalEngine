#include "component_gizmos.h"

#include <glm/gtc/type_ptr.hpp>

namespace ComponentGizmos {
// TODO: Implement IMGizmo debug drawing
// This will render 3D debug shapes (wireframes, icons) for various components

// stub:
/*
void DrawCamera(IMGizmo& gizmos, TransformComponent& transform, CameraComponent&
camera) { if (!camera.enabled_) return;

  gizmos.foreground = true;
  gizmos.color = EditorGizmoColor::CAMERA;
  gizmos.opacity = 0.8f;
  gizmos.boxWire(Transform::GetPosition(transform, Space::WORLD),
                 glm::vec3(2.0f),
                 Transform::GetRotation(transform, Space::WORLD));
}
*/
}  // namespace ComponentGizmos