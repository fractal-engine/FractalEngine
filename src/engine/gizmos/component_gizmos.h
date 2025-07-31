#ifndef COMPONENT_GIZMOS_H
#define COMPONENT_GIZMOS_H

#include "engine/ecs/world.h"
#include <imgui.h>
#include "engine/vendor/imguizmo/ImGuizmo.h" // Direct include

namespace ComponentGizmos {
    // The main function called by the renderer to draw the transform gizmo.
    void DrawTransformGizmo(Entity selectedEntity, const float* viewMatrix, const float* projectionMatrix);
}

#endif // COMPONENT_GIZMOS_H