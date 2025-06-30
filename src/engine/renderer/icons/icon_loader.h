#ifndef ICON_LOADER_H
#define ICON_LOADER_H

#include <imgui.h>
#include <filesystem>
#include <string_view>

namespace IconLoader {

// Load all icons from directory immediately
void LoadIcons(const std::filesystem::path& dir);

// Load all icons in background thread
void LoadIconsAsync(const std::filesystem::path& dir);

// Retrieve texture handle for specified icon by name
uint32_t GetIconHandle(const std::string& identifier);

// Create icon placeholder for specified file type
void CreatePlaceholderIcon(const std::filesystem::path& file);

// Convert icon handle to ImGui texture ID
inline ImTextureID ToImGuiTexture(const std::string& id) {
  return static_cast<ImTextureID>(
      static_cast<uintptr_t>(GetIconHandle(id)));
}

}  // namespace IconLoader

#endif  // ICON_LOADER_H