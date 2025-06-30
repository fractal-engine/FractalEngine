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
  std::filesystem::path cwd_ =
      std::filesystem::current_path();  // define current directory
  char filter_[128]{};                  // search filter for assets

  // Helper function: extracts filename from path for display
  static std::string Filename(const std::filesystem::directory_entry& e) {
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
      for (const auto& part : cwd_) {
        if (!first) {
          ImGui::SameLine();
          ImGui::TextUnformatted(">");
          ImGui::SameLine();
        }
        first = false;
        accumulator /= part;
        if (ImGui::Selectable(part.string().c_str(), false))
          cwd_ = accumulator;  // jump to selected path level
      }
      ImGui::Separator();
    }

    // ----- SEARCH FILTER -----
    ImGui::InputTextWithHint("##search", "Search content", filter_,
                             sizeof(filter_));
    ImGui::Separator();

    // ----- PARENT DIRECTORY NAVIGATION -----
    if (cwd_.has_parent_path()) {
      if (ImGui::Selectable("..", false))
        cwd_ = cwd_.parent_path();  // move one level up
      ImGui::Separator();
    }

    // ----- FILE/FOLDER COLLECTION & FILTERING -----
    std::vector<std::filesystem::directory_entry> entries;
    for (auto& e : std::filesystem::directory_iterator(cwd_)) {

      // Apply search filter
      if (filter_[0] != '\0') {
        std::string lower = Filename(e);
        std::string key = filter_;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        if (lower.find(key) == std::string::npos)
          continue;  // Skip non-matching entries
      }
      entries.push_back(e);
    }

    // ----- GRID LAYOUT DISPLAY -----
    constexpr int kCols = 7;             // columns in grid
    constexpr int kRows = 2;             // rows in grid
    constexpr ImVec2 cell_size{74, 74};  // item cell size
    const int max_items = kCols * kRows;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {8, 8});
    if (ImGui::BeginTable("grid", kCols, ImGuiTableFlags_SizingFixedFit)) {

      int shown = 0;
      for (auto& e : entries) {
        if (shown++ >= max_items)
          break;  // Limit number of items displayed
        ImGui::TableNextColumn();
        ImGui::PushID(e.path().c_str());  // unique ID for ImGui state

        bool is_dir = e.is_directory();

        // ----- INTERACTIVE AREA -----
        // draw invisible button for interaction area
        ImVec2 p0 = ImGui::GetCursorScreenPos();      // Item top-left position
        ImGui::InvisibleButton("##icon", cell_size);  // Clickable area
        bool hovered = ImGui::IsItemHovered();
        bool active = ImGui::IsItemActive();

        // ----- VISUAL HIGHLIGHTING -----
        // Draw hover/active highlight background
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (hovered || active) {
          ImU32 col = ImGui::GetColorU32(active ? ImGuiCol_ButtonActive
                                                : ImGuiCol_ButtonHovered);
          dl->AddRectFilled(p0, ImVec2(p0.x + cell_size.x, p0.y + cell_size.y),
                            col, 4.0f);  // Rounded corners
        }

        // ----- ICON RENDERING -----
        const char* tex_name = is_dir ? "folder" : "file";  // icon id
        ImTextureID tex_id =
            IconLoader::ToImGuiTexture(tex_name);  // GPU texture
        ImVec2 icon_size = {56.0f, 56.0f};         // draw size

        // centre the texture inside cell
        ImVec2 icon_pos = {p0.x + (cell_size.x - icon_size.x) * 0.5f,
                           p0.y + (cell_size.y - icon_size.y) * 0.5f};

        dl->AddImageRounded(
            tex_id, icon_pos,
            {icon_pos.x + icon_size.x, icon_pos.y + icon_size.y}, {0, 0},
            {1, 1}, IM_COL32_WHITE,
            6.0f);  // rounded corners

        // ----- HANDLE INTERACTION -----
        // Double-click to navigate or open
        if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
          if (is_dir)
            cwd_ /= e.path().filename();  // enter directory
          // TODO: Open file in appropriate editor
        }

        // ----- FILENAME LABEL -----
        // Position text below icon
        ImGui::SetCursorScreenPos(ImVec2(
            p0.x, p0.y + cell_size.y + ImGui::GetStyle().ItemInnerSpacing.y));
        ImGui::TextWrapped("%s", Filename(e).c_str());  // Wrap text if long

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
  static AssetBrowserPanel panel_;  // panel instance
  panel_.Draw();
}

}  // namespace Panels
#endif  // ASSET_BROWSER_H