#ifndef ASSET_BROWSER_H
#define ASSET_BROWSER_H

#include "engine/renderer/icons/icon_loader.h"

#include <imgui.h>
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace Panels {

struct AssetBrowserPanel {
  std::filesystem::path cwd =
      std::filesystem::current_path();  // define current directory
  char filter[128]{};                   // search filter for assets

  // Helper function: extracts filename from path for display
  static std::string filename(const std::filesystem::directory_entry& e) {
    return e.path().filename().string();
  }

  // Draw panel
  void Draw() {
    if (!ImGui::Begin("Asset Browser")) {
      ImGui::End();
      return;
    }

    // ----- BREADCRUMB NAVIGATION BAR -----
    {
      ImGui::TextDisabled("Path:");
      ImGui::SameLine();

      std::filesystem::path accumulator;
      bool first = true;

      // Render clickable path levels
      for (const auto& part : cwd) {
        if (!first) {
          ImGui::SameLine();
          ImGui::TextUnformatted(">");
          ImGui::SameLine();
        }
        first = false;
        accumulator /= part;
        if (ImGui::Selectable(part.string().c_str(), false))
          cwd = accumulator;  // jump to selected path level
      }
      ImGui::Separator();
    }

    // ----- SEARCH FILTER -----
    ImGui::InputTextWithHint("##search", "Search content", filter,
                             sizeof(filter));
    ImGui::Separator();

    // ----- PARENT DIRECTORY NAVIGATION -----
    if (cwd.has_parent_path()) {
      if (ImGui::Selectable("..", false))
        cwd = cwd.parent_path();  // move one level up
      ImGui::Separator();
    }

    // ----- FILE/FOLDER COLLECTION & FILTERING -----
    std::vector<std::filesystem::directory_entry> entries;
    for (auto& e : std::filesystem::directory_iterator(cwd)) {

      // Apply search filter
      if (filter[0] != '\0') {
        std::string lower = filename(e);
        std::string key = filter;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        if (lower.find(key) == std::string::npos)
          continue;  // Skip non-matching entries
      }
      entries.push_back(e);
    }

    // ----- GRID LAYOUT DISPLAY -----
    constexpr int kCols = 7;            // columns in grid
    constexpr int kRows = 2;            // rows in grid
    constexpr ImVec2 cellSize{74, 74};  // item cell size
    const int maxItems = kCols * kRows;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {8, 8});
    if (ImGui::BeginTable("grid", kCols, ImGuiTableFlags_SizingFixedFit)) {

      int shown = 0;
      for (auto& e : entries) {
        if (shown++ >= maxItems)
          break;  // Limit number of items displayed
        ImGui::TableNextColumn();
        ImGui::PushID(e.path().c_str());  // unique ID for ImGui state

        bool isDir = e.is_directory();

        // ----- INTERACTIVE AREA -----
        // draw invisible button for interaction area
        ImVec2 p0 = ImGui::GetCursorScreenPos();     // Item top-left position
        ImGui::InvisibleButton("##icon", cellSize);  // Clickable area
        bool hovered = ImGui::IsItemHovered();
        bool active = ImGui::IsItemActive();

        // ----- VISUAL HIGHLIGHTING -----
        // Draw hover/active highlight background
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (hovered || active) {
          ImU32 col = ImGui::GetColorU32(active ? ImGuiCol_ButtonActive
                                                : ImGuiCol_ButtonHovered);
          dl->AddRectFilled(p0, ImVec2(p0.x + cellSize.x, p0.y + cellSize.y),
                            col, 4.0f);  // Rounded corners
        }

        // ----- ICON RENDERING -----
        const char* texName = isDir ? "folder" : "file";          // icon id
        ImTextureID texId = IconLoader::toImGuiTexture(texName);  // GPU texture
        ImVec2 iconSize = {56.0f, 56.0f};                         // draw size

        // centre the texture inside cell
        ImVec2 iconPos = {p0.x + (cellSize.x - iconSize.x) * 0.5f,
                          p0.y + (cellSize.y - iconSize.y) * 0.5f};

        dl->AddImageRounded(texId, iconPos,
                            {iconPos.x + iconSize.x, iconPos.y + iconSize.y},
                            {0, 0}, {1, 1}, IM_COL32_WHITE,
                            6.0f);  // rounded corners

        // ----- HANDLE INTERACTION -----
        // Double-click to navigate or open
        if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
          if (isDir)
            cwd /= e.path().filename();  // enter directory
          // TODO: Open file in appropriate editor
        }

        // ----- FILENAME LABEL -----
        // Position text below icon
        ImGui::SetCursorScreenPos(ImVec2(
            p0.x, p0.y + cellSize.y + ImGui::GetStyle().ItemInnerSpacing.y));
        ImGui::TextWrapped("%s", filename(e).c_str());  // Wrap text if long

        ImGui::PopID();
      }
      ImGui::EndTable();
    }
    ImGui::PopStyleVar();

    ImGui::End();
  }
};

// Singleton accessor for EditorLayer
inline void AssetBrowser() {
  static AssetBrowserPanel p;  // panel instance
  p.Draw();
}

}  // namespace Panels
#endif  // ASSET_BROWSER_H
