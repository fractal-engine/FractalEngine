#include "asset_browser.h"

#include <imgui.h>
#include <algorithm>
#include <vector>

#include "editor/gui/inspectables/asset_inspectable.h"
#include "editor/runtime/application.h"
#include "engine/core/logger.h"
#include "engine/renderer/icons/icon_loader.h"

namespace Panels {

uint32_t AssetBrowserPanel::selected_folder_id_ = 0;
uint32_t AssetBrowserPanel::selected_node_id_ = 0;

AssetBrowserPanel::AssetBrowserPanel()
    : observer_(Application::Project().GetObserver()),
      assets_(Application::Project().Assets()) {}

// Extract filename from path for display
std::string AssetBrowserPanel::Filename(
    const std::filesystem::directory_entry& e) {
  return e.path().filename().string();
}

AssetBrowserPanel::NodeUIData AssetBrowserPanel::NodeUIData::CreateFor(
    const std::string& name, float scale) {
  // crop long names
  std::string label = name;
  constexpr uint32_t max_chars = 12;
  if (label.length() > max_chars + 3)
    label = label.substr(0, max_chars) + "...";

  // Calculate layout metrics
  ImFont* font = ImGui::GetFont();

  ImVec2 padding{14.0f * scale, 10.0f * scale};
  ImVec2 icon_size{64.0f * scale, 64.0f * scale};

  // Measure text dimensions at current font size
  ImVec2 text_size =
      font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, label.c_str());

  // Calculate total cell dimensions with padding
  const float total_w = icon_size.x + padding.x * 2.0f + 13.0f;
  const float total_h =
      icon_size.y + padding.y * 2.0f + text_size.y + padding.y;

  return NodeUIData(label, font, padding, icon_size, text_size,
                    ImVec2(total_w, total_h));
}

AssetBrowserPanel::NodeUIData AssetBrowserPanel::MakeNodeUI(
    const std::string& name) const {
  return NodeUIData::CreateFor(name, icon_scale_);
}

// Draw panel
void AssetBrowserPanel::Draw() {
  HandleInputs();  // called once per frame

  if (!ImGui::Begin("Asset Browser")) {
    ImGui::End();
    return;
  }

  // Get draw list and position
  ImDrawList& draw_list = *ImGui::GetWindowDrawList();
  ImVec2 position = ImGui::GetCursorScreenPos();
  ImVec2 size = ImGui::GetContentRegionAvail();

  // Render components
  ImVec2 folder_size = RenderSideFolders(draw_list, position);
  position.x += folder_size.x;

  ImVec2 nav_size = RenderBreadcrumbBar(draw_list, position);
  position.y += nav_size.y;

  ImVec2 content_size = ImVec2(size.x - folder_size.x, size.y - nav_size.y);

  RenderNodes(draw_list, position, content_size);

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

  // Capture input
  if (window_focused && io.KeyCtrl && io.MouseWheel != 0.0f) {
    const float step = icon_scale_ * scale_speed;
    target_icon_scale_ += (io.MouseWheel > 0.0f ? +step : -step);
  }

  // Clamp target and keep it within bounds
  target_icon_scale_ = std::clamp(target_icon_scale_, scale_min, scale_max);

  // Interpolation
  const float dt = io.DeltaTime;
  const float lerp = 1.0f - std::exp(-scale_smoothing * dt);  // exp-decay
  icon_scale_ += (target_icon_scale_ - icon_scale_) * lerp;
}

void AssetBrowserPanel::SelectFolder(uint32_t folder_id) {
  selected_folder_id_ = folder_id;
  selected_node_id_ = 0;
}

void AssetBrowserPanel::UnselectFolder() {
  SelectFolder(0);
}

void AssetBrowserPanel::OpenNodeInApplication(NodeRef node) {
  if (!node)
    return;

  // Get absolute path from project
  std::filesystem::path abs_path =
      Application::Project().AbsolutePath(node->path_);

  // Create system command based on platform
  std::string command;

#if defined(_WIN32) || defined(_WIN64)
  command = "start \"\" \"" + abs_path.string() + "\"";
#elif defined(__APPLE__)
  command = "open \"" + abs_path.string() + "\"";
#elif defined(__linux__)
  command = "xdg-open \"" + abs_path.string() + "\"";
#endif

  // Execute command if valid
  if (!command.empty()) {
    int result = std::system(command.c_str());
    if (result != 0) {
      Logger::getInstance().Log(LogLevel::Warning,
                                "Failed to open file: " + abs_path.string());
    }
  }
}

void AssetBrowserPanel::OpenNodeInExplorer(NodeRef node) {
  if (!node)
    return;

  // Get absolute path to parent directory
  std::filesystem::path dir_path =
      Application::Project().AbsolutePath(node->path_.parent_path());

  // Create system command based on platform
  std::string command;

#if defined(_WIN32) || defined(_WIN64)
  command = "explorer \"" + dir_path.string() + "\"";
#elif defined(__APPLE__)
  command = "open \"" + dir_path.string() + "\"";
#elif defined(__linux__)
  command = "xdg-open \"" + dir_path.string() + "\"";
#endif

  // Execute command if valid
  if (!command.empty()) {
    int result = std::system(command.c_str());
    if (result != 0) {
      Logger::getInstance().Log(
          LogLevel::Warning, "Failed to open directory: " + dir_path.string());
    }
  }
}

// ------------------------- TOP LEFT -------------------------
ImVec2 AssetBrowserPanel::RenderBreadcrumbBar(ImDrawList& draw_list,
                                              ImVec2 position) {
  const float height = 40.0f;
  const float padding = 40.0f;

  ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, height);
  ImVec2 p0 = position;
  ImVec2 p1 = ImVec2(p0.x + size.x, p0.y + size.y);

  // Get selected folder
  auto folder = observer_.FetchFolder(selected_folder_id_);

  // Draw back button if folder selected
  if (folder) {
    ImVec2 button_pos = ImVec2(position.x + 14.0f, position.y + 13.0f);
    if (ImGui::GetIO().MousePos.x >= button_pos.x &&
        ImGui::GetIO().MousePos.x <= button_pos.x + 20.0f &&
        ImGui::GetIO().MousePos.y >= button_pos.y &&
        ImGui::GetIO().MousePos.y <= button_pos.y + 20.0f) {

      if (ImGui::IsMouseClicked(0)) {
        uint32_t parent_id = folder->parent_id_;
        if (parent_id)
          SelectFolder(parent_id);
      }
    }
  }

  // Draw breadcrumb path
  std::string path_text;
  if (folder) {
    // Get project name
    std::string project_name = Application::Project().ProjectName();

    // Make folder path relative to project root
    const std::filesystem::path project_root =
        Application::Project().AbsolutePath("").lexically_normal();
    std::filesystem::path relative_path =
        std::filesystem::relative(folder->path_, project_root);

    // Build string
    std::string formatted_path;
    for (const auto& part : relative_path) {
      if (part == "." || part == "..")  // ignore dirs
        continue;

      if (!formatted_path.empty())
        formatted_path += " > ";

      formatted_path += part.string();
    }

    // Combine project name with formatted path
    path_text = formatted_path.empty() ? project_name
                                       : project_name + " > " + formatted_path;

  } else {
    // No folder selected
    path_text = "No folder selected";
  }

  // TODO: replace later with DynamicText()
  ImFont* f = ImGui::GetFont();
  const float big = f->FontSize * 1.15f;

  draw_list.AddText(
      f, big, ImVec2(position.x + padding, position.y + (height - big) * 0.5f),
      IM_COL32(255, 255, 255, 255), path_text.c_str());

  return size;
}

// --------------------- LEFT CONTAINER ---------------------
ImVec2 AssetBrowserPanel::RenderSideFolders(ImDrawList& draw_list,
                                            ImVec2 position) {
  const float width = 150.0f;

  ImVec2 size = ImVec2(width, ImGui::GetContentRegionAvail().y);
  ImVec2 p0 = position;
  ImVec2 p1 = ImVec2(p0.x + size.x, p0.y + size.y);

  // --- Folder tree ---
  ImVec2 list_pos = ImVec2(position.x, position.y);
  ImVec2 list_size = ImVec2(size.x, size.y);
  ImGui::SetCursorScreenPos(list_pos);
  ImGui::BeginChild("FolderTree", list_size, false);

  ImDrawList& tree_draw = *ImGui::GetWindowDrawList();

  // background color
  tree_draw.AddRectFilled(p0, p1, IM_COL32(25, 26, 28, 255), 4.0f);

  ImGui::Dummy(ImVec2(0, 8));

  auto root = observer_.RootNode();
  if (root)
    for (auto& sub : root->subfolders_)
      RenderSideFolder(tree_draw, sub, 0);

  ImGui::EndChild();
  ImGui::SetCursorScreenPos(p0);

  return size;
}

void AssetBrowserPanel::RenderSideFolder(ImDrawList& draw_list,
                                         FolderRef folder,
                                         uint32_t indentation) {
  if (!folder)
    return;

  // Calculate indentation offset
  const float indent_width = 20.0f;
  const float item_height = ImGui::GetFrameHeight();
  const float text_padding = 4.0f;

  // Get current cursor position
  ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
  cursor_pos.x += indent_width * indentation;

  // Item properties
  const bool has_children = !folder->subfolders_.empty();
  const bool selected = folder->id_ == selected_folder_id_;
  ImVec2 item_size =
      ImVec2(ImGui::GetContentRegionAvail().x - indent_width * indentation,
             item_height);

  // Draw item background
  ImU32 bg_color = selected ? IM_COL32(65, 85, 160, 180) : IM_COL32(0, 0, 0, 0);
  if (ImGui::IsMouseHoveringRect(
          cursor_pos,
          ImVec2(cursor_pos.x + item_size.x, cursor_pos.y + item_size.y)) &&
      !selected) {
    bg_color = IM_COL32(60, 60, 60, 80);
  }

  draw_list.AddRectFilled(
      cursor_pos,
      ImVec2(cursor_pos.x + item_size.x, cursor_pos.y + item_size.y), bg_color);

  // Draw folder icon
  float icon_size = item_height * 0.7f;
  ImVec2 icon_pos = ImVec2(cursor_pos.x + text_padding,
                           cursor_pos.y + (item_height - icon_size) * 0.5f);

  // Use IconLoader for folder icon
  uint32_t icon_handle = IconLoader::GetIconHandle("folder");
  draw_list.AddImage((ImTextureID)(intptr_t)icon_handle, icon_pos,
                     ImVec2(icon_pos.x + icon_size, icon_pos.y + icon_size));

  // Draw folder name
  ImVec2 text_pos =
      ImVec2(icon_pos.x + icon_size + 8.0f,
             cursor_pos.y + (item_height - ImGui::GetTextLineHeight()) * 0.5f);

  // Add caret for folders with children
  std::string folder_text = folder->name_;
  if (has_children) {
    folder_text = (folder->expanded_ ? "▼ " : "► ") + folder_text;
  }

  draw_list.AddText(text_pos, IM_COL32(220, 220, 220, 255),
                    folder_text.c_str());

  // Handle interactions
  ImGui::SetCursorScreenPos(cursor_pos);
  ImGui::InvisibleButton(("folder_" + std::to_string(folder->id_)).c_str(),
                         item_size);

  if (ImGui::IsItemClicked()) {
    // Select this folder
    SelectFolder(folder->id_);
  } else if (ImGui::IsItemClicked(1) && has_children) {
    // Right-click to toggle expansion
    const_cast<ProjectObserver::Folder*>(folder.get())->expanded_ =
        !folder->expanded_;
  }

  // Advance cursor for next item
  ImGui::SetCursorScreenPos(ImVec2(cursor_pos.x - indent_width * indentation,
                                   cursor_pos.y + item_height));

  // Render subfolders if expanded
  if (has_children && folder->expanded_) {
    for (const auto& subfolder : folder->subfolders_) {
      RenderSideFolder(draw_list, subfolder, indentation + 1);
    }
  }
}

// --------------------- RIGHT CONTAINER ---------------------
void AssetBrowserPanel::RenderNodes(ImDrawList& draw_list, ImVec2 position,
                                    ImVec2 size) {
  // Get current folder
  auto folder = observer_.FetchFolder(selected_folder_id_);
  if (!folder)
    return;

  // Make content area scrollable
  ImGui::SetCursorScreenPos(position);
  ImGui::BeginChild("ContentArea", size, false);

  // Setup grid layout
  const ImVec2 padding(22.0f, 22.0f);
  const ImVec2 spacing(16.0f, 16.0f);

  // Start position with padding
  ImVec2 cursor_pos = ImGui::GetCursorPos();
  cursor_pos.x += padding.x;
  cursor_pos.y += padding.y;
  ImVec2 grid_max = ImVec2(size.x - padding.x, 0);  // max width

  // Render all folders first
  for (const auto& subfolder : folder->subfolders_) {
    // Create UI data for this node
    NodeUIData ui_data = MakeNodeUI(subfolder->name_);

    // Check if we need to wrap to next row
    if (cursor_pos.x + ui_data.total_size.x > grid_max.x) {
      cursor_pos.x = padding.x;
      cursor_pos.y += ui_data.total_size.y + spacing.y;
    }

    // Set cursor and render node
    ImGui::SetCursorPos(cursor_pos);
    ImVec2 screen_pos = ImGui::GetCursorScreenPos();

    if (RenderNode(draw_list, subfolder, ui_data, screen_pos)) {
      // If rendered successfully, advance cursor
      cursor_pos.x += ui_data.total_size.x + spacing.x;
    }
  }

  // Then render all files
  for (const auto& file : folder->files_) {
    // Create UI data for this node
    NodeUIData ui_data = MakeNodeUI(file->name_);

    // Check if we need to wrap to next row
    if (cursor_pos.x + ui_data.total_size.x > grid_max.x) {
      cursor_pos.x = padding.x;
      cursor_pos.y += ui_data.total_size.y + spacing.y;
    }

    // Set cursor and render node
    ImGui::SetCursorPos(cursor_pos);
    ImVec2 screen_pos = ImGui::GetCursorScreenPos();

    if (RenderNode(draw_list, file, ui_data, screen_pos)) {
      // If rendered successfully, advance cursor
      cursor_pos.x += ui_data.total_size.x + spacing.x;
    }
  }

  ImGui::EndChild();
}

bool AssetBrowserPanel::RenderNode(ImDrawList& draw_list, NodeRef node,
                                   const NodeUIData& ui_data, ImVec2 position) {
  if (!node)
    return false;

  // Skip file nodes without assets
  if (!node->IsFolder()) {
    auto file = std::static_pointer_cast<const ProjectObserver::File>(node);
    if (!file->asset_id_)
      return false;
  }

  // Interactive area
  ImVec2 p0 = position;
  ImVec2 p1 = ImVec2(position.x + ui_data.total_size.x,
                     position.y + ui_data.total_size.y);

  ImGui::SetCursorScreenPos(position);
  ImGui::InvisibleButton(("node_" + std::to_string(node->id_)).c_str(),
                         ui_data.total_size);

  bool hovered = ImGui::IsItemHovered();
  bool clicked = ImGui::IsItemClicked();
  bool double_clicked =
      ImGui::IsItemClicked() && ImGui::GetIO().MouseDoubleClicked[0];
  bool selected = node->id_ == selected_node_id_;
  bool is_folder = node->IsFolder();

  // visual highligting
  ImU32 bg_color = IM_COL32(0, 0, 0, 0);
  if (selected)
    bg_color = IM_COL32(65, 85, 160, 180);
  else if (hovered)
    bg_color = IM_COL32(60, 60, 60, 80);

  draw_list.AddRectFilled(p0, p1, bg_color, 4.0f);

  // icon rendering
  ImVec2 icon_pos =
      ImVec2(position.x + (ui_data.total_size.x - ui_data.icon_size.x) * 0.5f,
             position.y + ui_data.padding.y);

  // Get appropriate icon
  uint32_t icon_handle;
  bool is_loading = false;

  if (node->IsFolder()) {
    // Use folder icon
    icon_handle = IconLoader::GetIconHandle("folder");
  } else {
    // Get file node and asset
    auto file = std::static_pointer_cast<const ProjectObserver::File>(node);
    auto asset = assets_.Get(file->asset_id_);

    if (asset) {
      // Use asset's icon if available
      icon_handle = asset->GetIcon();
      is_loading = asset->IsLoading();
    } else {
      // Fallback to generic file icon
      icon_handle = IconLoader::GetIconHandle("file");
    }
  }

  // Draw icon with white color
  ImU32 icon_color =
      is_loading ? IM_COL32(255, 255, 255, 180) : IM_COL32(255, 255, 255, 255);
  draw_list.AddImage((ImTextureID)(intptr_t)icon_handle, icon_pos,
                     ImVec2(icon_pos.x + ui_data.icon_size.x,
                            icon_pos.y + ui_data.icon_size.y),
                     ImVec2(0, 0), ImVec2(1, 1), icon_color);

  // label rendering
  ImVec2 text_pos = ImVec2(
      position.x + (ui_data.total_size.x - ui_data.text_size.x) * 0.5f,
      position.y + ui_data.padding.y + ui_data.icon_size.y + ui_data.padding.y);

  draw_list.AddText(ui_data.font, ui_data.font->FontSize, text_pos,
                    IM_COL32(220, 220, 220, 255), ui_data.text.c_str());

  // Handle interactions
  if (clicked) {
    // Select this node
    selected_node_id_ = node->id_;

    // If it's a file, create and show inspector
    if (!node->IsFolder()) {
      auto file = std::static_pointer_cast<const ProjectObserver::File>(node);
      if (file->asset_id_) {
        // TODO: Show asset in inspector
        // InsightPanelWindow::showAsset(file->asset_id_);
      }
    }
  }

  if (double_clicked) {
    if (node->IsFolder()) {
      // Enter folder
      SelectFolder(node->id_);
    } else {
      // Open file in application
      OpenNodeInApplication(node);
    }
  }

  // Handle middle click to open in explorer
  if (ImGui::IsItemClicked(2)) {
    OpenNodeInExplorer(node);
  }

  return true;
}

// Singleton accessor for EditorUI
void AssetBrowser() {
  static AssetBrowserPanel panel_;  // panel instance
  panel_.Draw();
}

}  // namespace Panels
