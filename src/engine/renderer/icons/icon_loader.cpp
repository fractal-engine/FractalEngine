#include "icon_loader.h"

#include <bgfx/bgfx.h>
#include <stb/stb_image.h>

#include <array>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "engine/core/logger.h"

namespace IconLoader::Internal {

using Path = std::filesystem::path;
using TextureHandle = bgfx::TextureHandle;

// file extensions
constexpr std::array<std::string_view, 3> kValidExt{".png", ".jpg", ".jpeg"};

// Map of icon identifiers to their texture handles
std::unordered_map<std::string, TextureHandle> gIcons;

// default texture, used when an icon is not found
TextureHandle gFallback{bgfx::kInvalidHandle};

// Protect concurrent access to shared icon registry
std::mutex gMutex;

// Helpers
[[nodiscard]] bool isSupported(const Path& p) noexcept {
  const auto ext = p.extension().string();
  return std::ranges::any_of(kValidExt,
                             [&](std::string_view v) { return ext == v; });
}

// Load image and upload to GPU as texture
[[nodiscard]] TextureHandle uploadTexture(const Path& file) {
  int w, h, comp;
  stbi_uc* pixels = stbi_load(file.string().c_str(), &w, &h, &comp, 4);
  if (!pixels) {
    Logger::getInstance().Log(
        LogLevel::Error, "IconLoader failed to load image: " + file.string());
    return {bgfx::kInvalidHandle};
  }

  // Copy pixel data to bgfx memory block
  const bgfx::Memory* mem = bgfx::copy(pixels, w * h * 4);
  stbi_image_free(pixels);

  // TODO: integrate with resource-manager instead of direct bgfx calls
  return bgfx::createTexture2D(
      static_cast<uint16_t>(w), static_cast<uint16_t>(h), false, 1,
      bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP,
      mem);
}

// Iterate through directory, call onFile for each valid icon file
void scanDirectory(const Path& dir,
                   const std::function<void(const Path&)>& onFile) {
  for (const auto& e : std::filesystem::directory_iterator(dir))
    if (!e.is_directory() && isSupported(e.path()))
      onFile(e.path());
}

// Logic to load icons from a directory
void loadDirectoryImpl(const Path& dir, bool async) {
  Logger::getInstance().Log(
      LogLevel::Info,
      std::string("IconLoader: scanning '") + dir.string() + '\'');

  std::vector<Path> files;
  scanDirectory(dir, [&](const Path& p) { files.emplace_back(p); });

  auto worker = [files = std::move(files)]() {
    for (const auto& file : files) {
      const std::string id = file.stem().string();

      std::scoped_lock lock(gMutex);
      if (gIcons.contains(id))
        continue;  // already loaded

      if (TextureHandle th = uploadTexture(file); bgfx::isValid(th)) {
        gIcons.emplace(id, th);
        Logger::getInstance().Log(LogLevel::Debug,
                                  "IconLoader: loaded '" + id + '\'');
      }
    }
  };

  if (async)
    std::thread(worker).detach();
  else
    worker();
}
}  // namespace IconLoader::Internal

// ---------------------------------------------------------------------------
//                              PUBLIC  API
// ---------------------------------------------------------------------------
namespace IconLoader {
// Load all icons from directory immediately
void loadIcons(const std::filesystem::path& dir) {
  Internal::loadDirectoryImpl(dir, /*async =*/false);
}

// Load all icons from directory in background thread
void loadIconsAsync(const std::filesystem::path& dir) {
  Internal::loadDirectoryImpl(dir, /*async =*/true);
}

// Return GPU handle for a named icon, fallback if not found
uint32_t getIconHandle(const std::string& id) {
  std::scoped_lock lock(Internal::gMutex);

  if (auto it = Internal::gIcons.find(id); it != Internal::gIcons.end())
    return it->second.idx;

  return Internal::gFallback.idx;  // may be invalid if no placeholder yet
}

// Support function for inline helper in header
uint32_t get(const std::string& id) {
  return getIconHandle(id);
}

// Set fallback texture used when icon is not found
void createPlaceholderIcon(const std::filesystem::path& file) {
  if (auto th = Internal::uploadTexture(file); bgfx::isValid(th)) {
    std::scoped_lock lock(Internal::gMutex);
    Internal::gFallback = th;
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
