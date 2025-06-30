#include "icon_loader.h"
#include "engine/core/logger.h"
#include "engine/renderer/texture/texture2d.h"

#include <stb/stb_image.h>

#include <array>
#include <filesystem>
#include <mutex>
#include <ranges>
#include <thread>
#include <unordered_map>
#include <vector>

namespace IconLoader::Internal {

using Path = std::filesystem::path;
using TexturePtr = std::shared_ptr<Gfx::Texture>;

// file extensions
static constexpr std::array<std::string_view, 3> kValidExt{".png", ".jpg",
                                                           ".jpeg"};

// Map of icon identifiers to their texture handles
std::unordered_map<std::string, TexturePtr> g_icons;

// default texture, used when an icon is not found
TexturePtr g_fallback;

// Protect concurrent access to shared icon registry
std::mutex g_mutex;

// ─────────────── helpers ────────────────
[[nodiscard]] bool IsSupported(const Path& path) noexcept {
  const std::string ext = path.extension().string();
  return std::ranges::any_of(kValidExt,
                             [&](std::string_view v) { return ext == v; });
}

// Scan directory for supported image files, returns their paths
static std::vector<Path> CollectFiles(const Path& dir) {
  std::vector<Path> out;
  for (auto& entry : std::filesystem::directory_iterator(dir))
    if (!entry.is_directory() && IsSupported(entry.path()))
      out.emplace_back(entry.path());
  return out;
}

// Insert new icon into the global registry
inline void InsertIcon(const std::string& id, TexturePtr texture) {
  std::scoped_lock lock(g_mutex);
  g_icons.emplace(id, std::move(texture));
}

// ─────────────── main loader ────────────────
// Load icons from directory
void LoadDirectory(const Path& dir, bool async) {
  Logger::getInstance().Log(LogLevel::Info,
                            "IconLoader: scanning '" + dir.string() + '\'');

  auto files = CollectFiles(dir);

  // load each file
  auto worker = [files = std::move(files)]() {
    for (const auto& file_path : files) {
      const std::string id = file_path.stem().string();

      {
        std::scoped_lock lock(g_mutex);
        if (g_icons.contains(id))
          continue;  // Skip loaded icons
      }

      // Load texture through TextureCache, avoids duplicates
      auto texture = Gfx::TextureCache::Instance().Get(file_path, Gfx::TextureType::IMAGE);

      if (texture) {
        InsertIcon(id, texture);
        Logger::getInstance().Log(LogLevel::Debug,
                                  "IconLoader: loaded '" + id + '\'');
      } else {
        Logger::getInstance().Log(
            LogLevel::Warning,
            "IconLoader: failed to load icon '" + file_path.string() + '\'');
      }
    }
  };

  async ? std::thread(worker).detach() : worker();
}

}  // namespace IconLoader::Internal

// ---------------------------------------------------------------------------
//                              PUBLIC  API
// ---------------------------------------------------------------------------
namespace IconLoader {

// Load all icons from directory immediately
void LoadIcons(const std::filesystem::path& dir) {
  Internal::LoadDirectory(dir, /*async =*/false);
}

// Load all icons from directory in background thread
void LoadIconsAsync(const std::filesystem::path& dir) {
  Internal::LoadDirectory(dir, /*async =*/true);
}

// Return GPU handle for a named icon, fallback if not found
uint32_t GetIconHandle(const std::string& id) {
  std::scoped_lock lock(Internal::g_mutex);

  const auto it = Internal::g_icons.find(id);
  const auto& texture =
      (it != Internal::g_icons.end()) ? it->second : Internal::g_fallback;

  return texture && texture->Valid() ? texture->Handle().idx : bgfx::kInvalidHandle;
}

// Support function for inline helper in header
uint32_t Get(const std::string& id) {
  return GetIconHandle(id);
}

// Set fallback texture used when icon is not found
void CreatePlaceholderIcon(const std::filesystem::path& file) {
  auto texture = Gfx::TextureCache::Instance().Get(file, Gfx::TextureType::IMAGE);

  if (texture && texture->Valid()) {
    std::scoped_lock lock(Internal::g_mutex);
    Internal::g_fallback = std::move(texture);

    Logger::getInstance().Log(
        LogLevel::Info,
        "IconLoader: fallback icon set to '" + file.string() + '\'');
  } else {
    Logger::getInstance().Log(
        LogLevel::Error,
        "IconLoader: failed to load fallback icon '" + file.string() + '\'');
  }
}

}  // namespace IconLoader
