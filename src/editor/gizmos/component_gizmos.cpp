#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include "editor/gizmos/component_gizmos.h"

namespace ComponentGizmos {
static ImGuizmo::OPERATION current_operation = ImGuizmo::TRANSLATE;
static ImGuizmo::MODE current_mode = ImGuizmo::WORLD;

void DrawTransformGizmo(Entity selectedEntity, const float* viewMatrix,
                        const float* projectionMatrix) {
  if (ImGui::IsKeyPressed(ImGuiKey_W))
    current_operation = ImGuizmo::TRANSLATE;
  if (ImGui::IsKeyPressed(ImGuiKey_E))
    current_operation = ImGuizmo::ROTATE;
  if (ImGui::IsKeyPressed(ImGuiKey_R))
    current_operation = ImGuizmo::SCALE;

  auto& ecs = ECS::Main();
  if (selectedEntity == entt::null ||
      !ecs.Has<TransformComponent>(selectedEntity)) {
    return;
  }
  auto& transform = ecs.Get<TransformComponent>(selectedEntity);

  ImGuizmo::BeginFrame();
  const ImGuiIO& io = ImGui::GetIO();
  ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

  glm::mat4 modelMatrix = transform.model_;
  bool manipulated =
      ImGuizmo::Manipulate(viewMatrix, projectionMatrix, current_operation,
                           current_mode, glm::value_ptr(modelMatrix));

  if (manipulated && ImGuizmo::IsUsing()) {
    glm::vec3 p, r, s;
    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(modelMatrix),
                                          glm::value_ptr(p), glm::value_ptr(r),
                                          glm::value_ptr(s));
    transform.position_ = p;
    transform.euler_angles_ = r;
    transform.rotation_ = glm::quat(glm::radians(r));
    transform.scale_ = s;
    transform.modified_ = true;
  }
}
}  // namespace ComponentGizmos