#ifndef ASSET_BROWSER_H
#define ASSET_BROWSER_H

#include <imgui.h>
#include <filesystem>
#include <string>

namespace Panels {

class AssetBrowserPanel {

public:
  // Draw panel
  void Draw();

  static void SelectFolder(
      uint32_t folderId);  // placeholder, select folder by node id

  static void UnselectFolder();  // placeholder, unselect folder

private:
  // UI layout data
  struct NodeUIData {
    std::string text;
    ImFont* font;
    ImVec2 padding;
    ImVec2 icon_size;
    ImVec2 text_size;
    ImVec2 total_size;
  };

  // Prepare UI display data for node with specified name
  NodeUIData MakeNodeUI(const std::string& name) const;

  // ----- DATA -----
  std::filesystem::path cwd_{
      std::filesystem::current_path()};  // current directory
  char filter_[128]{};                   // search filter

  float icon_scale_{1.0f};         // on-screen scale
  float target_icon_scale_{1.0f};  // scale modifier

  // ----- UTILITIES -----
  void HandleInputs();  // Handle user input
  static std::string Filename(
      const std::filesystem::directory_entry& e);  // Extract filename from path
};

// Singleton accessor for EditorLayer
void AssetBrowser();

}  // namespace Panels
#endif  // ASSET_BROWSER_H
