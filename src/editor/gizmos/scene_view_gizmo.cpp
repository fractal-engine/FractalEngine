#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include "editor/gui/orbit_camera.h"
#include "editor/vendor/imguizmo/ImGuizmo.h"
#include "scene_view_gizmo.h"

SceneViewGizmo::SceneViewGizmo() {}

void SceneViewGizmo::OnRender(OrbitCamera& camera, Entity selectedEntity) {
  // 1. Handle keyboard input to change the operation
  if (ImGui::IsKeyPressed(ImGuiKey_W))
    m_operation = GizmoOperation::Translate;
  if (ImGui::IsKeyPressed(ImGuiKey_E))
    m_operation = GizmoOperation::Rotate;
  if (ImGui::IsKeyPressed(ImGuiKey_R))
    m_operation = GizmoOperation::Scale;

  // 2. Check if there is a valid entity selected
  auto& ecs = ECS::Main();
  if (selectedEntity == entt::null || !ecs.Reg().valid(selectedEntity)) {
    return;
  }

  auto& transform = ecs.Get<TransformComponent>(selectedEntity);

  // 3. Get camera matrices
  float viewMatrix[16];
  float projMatrix[16];
  camera.getViewMatrix(viewMatrix);
  camera.getProjectionMatrix(
      projMatrix, ImGui::GetIO().DisplaySize.x / ImGui::GetIO().DisplaySize.y);

  // 4. Set up ImGuizmo state
  ImGuizmo::BeginFrame();
  ImGuizmo::SetRect(0, 0, ImGui::GetIO().DisplaySize.x,
                    ImGui::GetIO().DisplaySize.y);

  // 5. Convert our operation enum to ImGuizmo's enum
  ImGuizmo::OPERATION imguizmoOp = ImGuizmo::TRANSLATE;
  if (m_operation == GizmoOperation::Rotate)
    imguizmoOp = ImGuizmo::ROTATE;
  if (m_operation == GizmoOperation::Scale)
    imguizmoOp = ImGuizmo::SCALE;

  // 6. Manipulate the matrix
  glm::mat4 modelMatrix = transform.model_;
  if (ImGuizmo::Manipulate(viewMatrix, projMatrix, imguizmoOp, ImGuizmo::WORLD,
                           glm::value_ptr(modelMatrix))) {
    // 7. If manipulated, decompose the matrix and update the component
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