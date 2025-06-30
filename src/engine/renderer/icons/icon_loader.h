#ifndef ICON_LOADER_H
#define ICON_LOADER_H

#include <imgui.h>
#include <filesystem>
#include <string_view>

namespace IconLoader {

// Load all icons from directory immediately
void loadIcons(const std::filesystem::path& dir);

// Load all icons in background thread
void loadIconsAsync(const std::filesystem::path& dir);

// Retrieve texture handle for specified icon by name
uint32_t getIconHandle(const std::string& identifier);

// Create icon placeholder for specified file type
void createPlaceholderIcon(const std::filesystem::path& file);

// Convert icon handle to ImGui texture ID
inline ImTextureID toImGuiTexture(const std::string& id) {
  return static_cast<ImTextureID>(
      static_cast<uintptr_t>(getIconHandle(id)));
}

}  // namespace IconLoader

#endif  // ICON_LOADER_H