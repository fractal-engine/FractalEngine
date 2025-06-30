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
std::unordered_map<std::string, TexturePtr> gIcons;

// default texture, used when an icon is not found
TexturePtr gFallback;

// Protect concurrent access to shared icon registry
std::mutex gMutex;

// ─────────────── helpers ────────────────
[[nodiscard]] bool isSupported(const Path& p) noexcept {
  const std::string ext = p.extension().string();
  return std::ranges::any_of(kValidExt,
                             [&](std::string_view v) { return ext == v; });
}

// Scan directory for supported image files, returns their paths
static std::vector<Path> collectFiles(const Path& dir) {
  std::vector<Path> out;
  for (auto& e : std::filesystem::directory_iterator(dir))
    if (!e.is_directory() && isSupported(e.path()))
      out.emplace_back(e.path());
  return out;
}

// Insert new icon into the global registry
inline void insertIcon(const std::string& id, TexturePtr tex) {
  std::scoped_lock lk(gMutex);
  gIcons.emplace(id, std::move(tex));
}

// ─────────────── main loader ────────────────
// Load icons from directory
void loadDirectory(const Path& dir, bool async) {
  Logger::getInstance().Log(LogLevel::Info,
                            "IconLoader: scanning '" + dir.string() + '\'');

  auto files = collectFiles(dir);

  // load each file
  auto worker = [files = std::move(files)]() {
    for (const auto& f : files) {
      const std::string id = f.stem().string();

      {
        std::scoped_lock lk(gMutex);
        if (gIcons.contains(id))
          continue;  // Skip loaded icons
      }

      // Load texture through TextureCache, avoids duplicates
      auto tex = Gfx::TextureCache::instance().get(f, Gfx::TextureType::IMAGE);

      if (tex) {
        insertIcon(id, tex);
        Logger::getInstance().Log(LogLevel::Debug,
                                  "IconLoader: loaded '" + id + '\'');
      } else {
        Logger::getInstance().Log(
            LogLevel::Warning,
            "IconLoader: failed to load icon '" + f.string() + '\'');
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
void loadIcons(const std::filesystem::path& dir) {
  Internal::loadDirectory(dir, /*async =*/false);
}

// Load all icons from directory in background thread
void loadIconsAsync(const std::filesystem::path& dir) {
  Internal::loadDirectory(dir, /*async =*/true);
}

// Return GPU handle for a named icon, fallback if not found
uint32_t getIconHandle(const std::string& id) {
  std::scoped_lock lk(Internal::gMutex);

  const auto it = Internal::gIcons.find(id);
  const auto& tex =
      (it != Internal::gIcons.end()) ? it->second : Internal::gFallback;

  return tex && tex->valid() ? tex->handle().idx : bgfx::kInvalidHandle;
}

// Support function for inline helper in header
uint32_t get(const std::string& id) {
  return getIconHandle(id);
}

// Set fallback texture used when icon is not found
void createPlaceholderIcon(const std::filesystem::path& file) {
  auto tex = Gfx::TextureCache::instance().get(file, Gfx::TextureType::IMAGE);

  if (tex && tex->valid()) {
    std::scoped_lock lk(Internal::gMutex);
    Internal::gFallback = std::move(tex);

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
