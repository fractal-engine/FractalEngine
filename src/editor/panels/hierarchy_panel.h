#ifndef HIERARCHY_PANEL_H
#define HIERARCHY_PANEL_H

#include <imgui.h>
#include <filesystem>
#include <string>
#include <vector>
#include "editor/editor_layer.h"

// -----------------------------------------------------------------------------
//  Panels::SceneHierarchy  – entity list   +    file explorer
// -----------------------------------------------------------------------------
namespace Panels {

inline void HierarchyPanel(const std::vector<std::string>& entityNames,
                           const std::filesystem::path& assetsRoot) {
  ImGui::Begin("Hierarchy", nullptr);

  //------------------------- ENTITIES ---------------------------------------
  ImGui::TextUnformatted("Scene");
  ImGui::Separator();

  for (int i = 0; i < static_cast<int>(entityNames.size()); ++i) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf |
                               ImGuiTreeNodeFlags_NoTreePushOnOpen |
                               ImGuiTreeNodeFlags_SpanAvailWidth;

    if (EditorLayer::Get()->GetSelectedEntity() == i)
      flags |= ImGuiTreeNodeFlags_Selected;

    ImGui::TreeNodeEx((void*)(intptr_t)i, flags, "%s", entityNames[i].c_str());

    if (ImGui::IsItemClicked()) {
      EditorLayer::Get()->SetSelectedEntity(i);
    }
  }

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

  if (std::filesystem::exists(assetsRoot))
    recurse(assetsRoot, 0);

  ImGui::End();
}

}  // namespace Panels
#endif  // HIERARCHY_PANEL_H
