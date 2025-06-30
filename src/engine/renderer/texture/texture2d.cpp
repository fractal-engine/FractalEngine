#include "texture2d.h"
#include <stb/stb_image.h>
#include "engine/core/logger.h"

using namespace Gfx;

namespace Gfx::TextureInternal {
// ----------------------------------------------------------------------
// Format information for texture configuration
// ----------------------------------------------------------------------
struct FormatInfo {
  bgfx::TextureFormat::Enum fmt;
  bool srgb;  // Whether texture needs sRGB color space conversion
};

// clang-format off
    static constexpr std::array<FormatInfo, 3> kLinearByChannels = {{
        /* 1 ch */ { bgfx::TextureFormat::R8,   false },
        /* 3 ch */ { bgfx::TextureFormat::RGB8, false },
        /* 4 ch */ { bgfx::TextureFormat::RGBA8,false }
    }};

    //
    static const std::unordered_map<TextureType, FormatInfo> kTypeTable = {
        { TextureType::ALBEDO,     { bgfx::TextureFormat::RGBA8, true  } }, // sRGB
        { TextureType::EMISSIVE,   { bgfx::TextureFormat::RGBA8, true  } }, // sRGB
        { TextureType::NORMAL,     { bgfx::TextureFormat::RGBA8, false } },
        { TextureType::ROUGHNESS,  { bgfx::TextureFormat::R8,    false } },
        { TextureType::METALLIC,   { bgfx::TextureFormat::R8,    false } },
        { TextureType::OCCLUSION,  { bgfx::TextureFormat::R8,    false } },
        { TextureType::HEIGHT,     { bgfx::TextureFormat::R8,    false } },
        { TextureType::IMAGE,      { bgfx::TextureFormat::RGBA8, false } }, // Regular image display
        { TextureType::EMPTY,      { bgfx::TextureFormat::RGBA8, false } }  // default
    };
// clang-format on

// ----------------------------------------------------------------------
//
// ----------------------------------------------------------------------
[[nodiscard]]
FormatInfo chooseFormat(TextureType type, int channels) {
  auto it = kTypeTable.find(type);
  FormatInfo info = (it != kTypeTable.end())
                        ? it->second
                        : kLinearByChannels[2];  // fallback RGBA8

  // For generic IMAGE type, choose format based on channel count
  if (type == TextureType::IMAGE) {
    channels = std::clamp(channels, 1, 4);
    info = kLinearByChannels[(channels == 1) ? 0
                             : (channels == 2)
                                 ? 0
                                 :  // Use R8 for both 1 and 2 channels
                                 (channels == 3) ? 1
                                                 : 2];
  }
  return info;
}
}  // namespace Gfx::TextureInternal

// Private constructors used to create textures with existing GPU handles
Texture::Texture(bgfx::TextureHandle h, uint16_t w, uint16_t hgt, TextureType k)
    : m_handle(h), m_width(w), m_height(hgt), m_type(k) {}

// Destructor
Texture::~Texture() {
  if (bgfx::isValid(m_handle))
    bgfx::destroy(m_handle);
}

// Move constructor transfers ownership of GPU resources
Texture::Texture(Texture&& o) noexcept {
  *this = std::move(o);
}

// Move assignment transfers ownership, release existing resources
Texture& Texture::operator=(Texture&& o) noexcept {
  if (this != &o) {
    if (bgfx::isValid(m_handle))
      bgfx::destroy(m_handle);
    m_handle = o.m_handle;
    m_width = o.m_width;
    m_height = o.m_height;
    m_type = o.m_type;
    o.m_handle = {bgfx::kInvalidHandle};
  }
  return *this;
}

// Method to load 2D texture from disk, uploads it to GPU
std::shared_ptr<Texture> Texture::load2D(
    const std::filesystem::path& file, TextureType type,
    uint64_t samplerFlags /* = BGFX_SAMPLER_NONE */) {

  // Load image data using from disk using stb_image
  int w{}, h{}, ch{};
  stbi_uc* px = stbi_load(file.string().c_str(), &w, &h, &ch, 0);
  if (!px) {
    Logger::getInstance().Log(
        LogLevel::Error, "Texture2D: failed to load '" + file.string() + '\'');
    return {};
  }

  // Copy pixel data to bgfx memory for GPU upload
  const bgfx::Memory* mem = bgfx::copy(px, w * h * 4);
  stbi_image_free(px);

  // Select appropriate format and flags based on texture type
  const auto fmtInfo = TextureInternal::chooseFormat(type, ch);
  const uint64_t flags =
      samplerFlags | (fmtInfo.srgb ? BGFX_TEXTURE_SRGB : 0ULL);

  // Create GPU texture with selected format and flags
  bgfx::TextureHandle hndl = bgfx::createTexture2D(
      uint16_t(w), uint16_t(h), false, 1, fmtInfo.fmt, flags, mem);

  if (!bgfx::isValid(hndl)) {
    Logger::getInstance().Log(
        LogLevel::Error,
        "bgfx::createTexture2D failed for '" + file.string() + '\'');
    return {};
  }

  Logger::getInstance().Log(LogLevel::Debug,
                            "Texture2D: uploaded '" + file.filename().string() +
                                "' (fmt= " + std::to_string(fmtInfo.fmt) + ')');
}

// Return singleton instance of TextureCache
TextureCache& TextureCache::instance() {
  static TextureCache cache;
  return cache;
}

// Get texture from cache or load it if not found
std::shared_ptr<Texture> TextureCache::get(const std::filesystem::path& file,
                                           TextureType type) {
  const std::string key = file.string();

  {
    std::scoped_lock lk(m_mtx);
    if (auto it = m_map.find(key); it != m_map.end())
      return it->second;
  }

  // If not found in cache, load texture from disk
  auto tex = Texture::load2D(file, type);
  if (tex) {
    std::scoped_lock lk(m_mtx);
    m_map.emplace(key, tex);
  }
  return tex;
}

// Clear the texture cache
void TextureCache::clear() {
  std::scoped_lock lk(m_mtx);
  m_map.clear();  // shared pointer, handle destruction of GPU resources
                  // TODO: attach to resource manager
}
