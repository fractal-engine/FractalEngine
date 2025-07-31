#ifndef HIERARCHY_PANEL_H
#define HIERARCHY_PANEL_H

#include <imgui.h>
#include <filesystem>
#include "editor/editor_ui.h"
#include "engine/ecs/world.h"  // We need this to access the ECS

namespace Panels {

inline void HierarchyPanel() {
  // We need to wrap the entire panel's content in Begin/End calls.
  ImGui::Begin("Hierarchy", nullptr);

  //------------------------- ENTITIES ---------------------------------------
  ImGui::TextUnformatted("Scene");
  ImGui::Separator();

  auto& ecs = ECS::Main();
  auto* editor = EditorUI::Get();

  ecs.View<TransformComponent>().each([&](auto entity, auto& transform) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf |
                               ImGuiTreeNodeFlags_NoTreePushOnOpen |
                               ImGuiTreeNodeFlags_SpanAvailWidth;

    if (editor->GetSelectedEntity() == entity) {
      flags |= ImGuiTreeNodeFlags_Selected;
    }

    std::string label =
        transform.name_.empty()
            ? "Entity " + std::to_string(entt::to_integral(entity))
            : transform.name_;

    ImGui::TreeNodeEx((void*)((intptr_t)entity), flags, "%s", label.c_str());

    if (ImGui::IsItemClicked()) {
      editor->SetSelectedEntity(entity);
    }
  });

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  //------------------------- FILES ------------------------------------------

  ImGui::TextUnformatted("Assets");
  ImGui::Separator();

  static int fileTreeSelection = -1;
  int nodeId = 0;

  std::function<void(const std::filesystem::path&, int)> recurse =
      [&](const std::filesystem::path& p, int depth) {
        // Error handling in case the path doesn't exist or isn't a directory
        if (!std::filesystem::exists(p) || !std::filesystem::is_directory(p)) {
          return;
        }
        for (auto& entry : std::filesystem::directory_iterator(p)) {
          const bool isDir = entry.is_directory();
          ImGuiTreeNodeFlags f = ImGuiTreeNodeFlags_SpanFullWidth;
          if (!isDir)
            f |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

          if (fileTreeSelection == nodeId)
            f |= ImGuiTreeNodeFlags_Selected;

          const std::string label = entry.path().filename().string();
          const bool opened = ImGui::TreeNodeEx((void*)(intptr_t)nodeId, f,
                                                "%s", label.c_str());

          if (ImGui::IsItemClicked())
            fileTreeSelection = nodeId;
          ++nodeId;

          if (isDir && opened) {
            recurse(entry.path(), depth + 1);
            ImGui::TreePop();
          }
        }
      };

  // Replaced the undefined 'assetsRoot' variable with a hardcoded
  // path for now
  const std::filesystem::path assetsRootPath = "assets";
  recurse(assetsRootPath, 0);

  ImGui::End();
}

}  // namespace Panels
#endif  // HIERARCHY_PANEL_H