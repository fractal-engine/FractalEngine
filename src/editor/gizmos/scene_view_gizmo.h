#ifndef SCENE_VIEW_GIZMO_H
#define SCENE_VIEW_GIZMO_H

#include <glm/glm.hpp>
#include "engine/ecs/world.h"

// Forward declare OrbitCamera
class OrbitCamera;

class SceneViewGizmo {
public:
  SceneViewGizmo();

  // The main function called by the pipeline each frame.
  void OnRender(OrbitCamera& camera, Entity selectedEntity);

private:
  // Internal state for the gizmo
  enum class GizmoOperation { Translate, Rotate, Scale };
  GizmoOperation m_operation = GizmoOperation::Translate;
};

#endif  // SCENE_VIEW_GIZMO_H