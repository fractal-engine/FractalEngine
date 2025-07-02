#ifndef ASSET_BROWSER_H
#define ASSET_BROWSER_H

#include <filesystem>
#include <string>

namespace Panels {

struct AssetBrowserPanel {
  std::filesystem::path cwd_{
      std::filesystem::current_path()};  // current directory
  char filter_[128]{};                   // search filter

  float icon_scale_{1.0f};         // on-screen scale
  float target_icon_scale_{1.0f};  // scale modifier

  // Handle user input
  void HandleInputs();

  static std::string Filename(const std::filesystem::directory_entry& e);

  // Draw panel
  void Draw();
};

// Singleton accessor for EditorLayer
void AssetBrowser();

}  // namespace Panels
#endif  // ASSET_BROWSER_H
