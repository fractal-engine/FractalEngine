#include "asset_browser.h"

#include "engine/renderer/icons/icon_loader.h"

#include <imgui.h>
#include <algorithm>
#include <vector>

namespace Panels {

// Helper function: extracts filename from path for display
std::string AssetBrowserPanel::Filename(
    const std::filesystem::directory_entry& e) {
  return e.path().filename().string();
}

// Draw panel
void AssetBrowserPanel::Draw() {
  HandleInputs();  // called once per frame

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
  constexpr int columns = 7;  // columns
  constexpr int rows = 2;     // rows

  // base sizes scaled by user input
  const ImVec2 cell_size{74.0f * icon_scale_, 74.0f * icon_scale_};
  const ImVec2 icon_base{56.0f, 56.0f};
  const int max_items = columns * rows;

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {8, 8});
  if (ImGui::BeginTable("grid", columns, ImGuiTableFlags_SizingFixedFit)) {

    int shown = 0;
    for (auto& e : entries) {
      if (shown++ >= max_items)
        break;  // Limit number of items displayed
      ImGui::TableNextColumn();
      ImGui::PushID(e.path().c_str());  // unique ID for ImGui state

      bool is_dir = e.is_directory();

      // ----- INTERACTIVE AREA -----
      ImVec2 p0 = ImGui::GetCursorScreenPos();      // Item top-left position
      ImGui::InvisibleButton("##icon", cell_size);  // Clickable area
      bool hovered = ImGui::IsItemHovered();
      bool active = ImGui::IsItemActive();

      // ----- VISUAL HIGHLIGHTING -----
      ImDrawList* dl = ImGui::GetWindowDrawList();
      if (hovered || active) {
        ImU32 col = ImGui::GetColorU32(active ? ImGuiCol_ButtonActive
                                              : ImGuiCol_ButtonHovered);
        dl->AddRectFilled(p0, ImVec2(p0.x + cell_size.x, p0.y + cell_size.y),
                          col, 4.0f);  // Rounded corners
      }

      // ----- ICON RENDERING -----
      ImTextureID texture_id = IconLoader::ToImGuiTexture(
          is_dir ? "folder" : "file");  // fallback icon if key missing
      ImVec2 icon_size = {icon_base.x * icon_scale_,
                          icon_base.y * icon_scale_};  // calculate size

      // Centre the texture inside cell
      ImVec2 icon_pos = {p0.x + (cell_size.x - icon_size.x) * 0.5f,
                         p0.y + (cell_size.y - icon_size.y) * 0.5f};

      dl->AddImageRounded(texture_id, icon_pos,
                          {icon_pos.x + icon_size.x, icon_pos.y + icon_size.y},
                          {0, 0}, {1, 1}, IM_COL32_WHITE,
                          6.0f);  // rounded corners

      // ----- HANDLE INTERACTION -----
      if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        if (is_dir)
          cwd_ /= e.path().filename();  // enter directory
        // TODO: Open file in specified editor
      }

      // ----- FILENAME LABEL -----
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

// Handle user input (zooming with Ctrl + wheel)
void AssetBrowserPanel::HandleInputs() {
  ImGuiIO& io = ImGui::GetIO();

  const float scale_speed = 0.10f;     // wheel sensitivity
  const float scale_smoothing = 6.5f;  // interpolation rate
  const float scale_min = 0.8f;        // hard limit
  const float scale_max = 4.0f;

  const bool window_focused =
      ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) ||
      ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

  // ── CAPTURE INPUT ────────────────────────────────────────────
  if (window_focused && io.KeyCtrl && io.MouseWheel != 0.0f) {
    const float step = icon_scale_ * scale_speed;
    target_icon_scale_ += (io.MouseWheel > 0.0f ? +step : -step);
  }

  // Clamp target and keep it within bounds
  target_icon_scale_ = std::clamp(target_icon_scale_, scale_min, scale_max);

  // ── INTERPOLATION ─────────────────
  const float dt = io.DeltaTime;
  const float lerp = 1.0f - std::exp(-scale_smoothing * dt);  // exp-decay
  icon_scale_ += (target_icon_scale_ - icon_scale_) * lerp;
}

// Singleton accessor for EditorLayer
void AssetBrowser() {
  static AssetBrowserPanel panel_;  // panel instance
  panel_.Draw();
}

}  // namespace Panels
