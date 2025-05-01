#ifndef INSPECTOR_PANEL_H
#define INSPECTOR_PANEL_H

#include <imgui.h>
#include <glm/glm.hpp>
#include "editor/editor_layer.h"

namespace Components {

struct Transform {
  glm::vec3 pos{0};
  glm::vec3 rot{0};
  glm::vec3 scl{1};
};

inline void Inspector(const std::vector<Transform>& transforms) {
  ImGui::Begin("Inspector");

  int sel = EditorLayer::Get()->GetSelectedEntity();
  if (sel < 0 || sel >= (int)transforms.size()) {
    ImGui::TextDisabled("Select an entity");
    ImGui::End();
    return;
  }

  const int id = sel;
  Transform t = transforms[id];  // local copy for editing

  ImGui::Text("Entity  #%d", id);
  ImGui::Separator();

  if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::DragFloat3("Position", &t.pos.x, 0.1f);
    ImGui::DragFloat3("Rotation", &t.rot.x, 1.0f);
    ImGui::DragFloat3("Scale", &t.scl.x, 0.05f);
  }

  // TODO: components: mesh-renderer, light, skybox, …

  // store edits back into ECS / scene graph here
  // Scene.SetTransform(id, t);

  ImGui::End();
}

}  // namespace Components
#endif  // INSPECTOR_PANEL_H
