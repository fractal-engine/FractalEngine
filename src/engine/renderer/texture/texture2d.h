#ifndef TEXTURE2D_H
#define TEXTURE2D_H

#include <bgfx/bgfx.h>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

// --------------------------------------------------------------------------
//  2D texture wrapper
//  -------------------------------------------------------------------------
namespace Gfx {

// Define texture types
enum class TextureType {
  IMAGE,
  ALBEDO,
  NORMAL,
  ROUGHNESS,
  METALLIC,
  OCCLUSION,
  EMISSIVE,
  HEIGHT,
  EMPTY  // uninitialised or invalid
};

// Manage GPU texture resources
class Texture {
public:
  Texture() = default;
  ~Texture();  // destroy GPU handle

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;
  Texture(Texture&&) noexcept;
  Texture& operator=(Texture&&) noexcept;

  // Accessors for texture state
  [[nodiscard]] bool valid() const { return bgfx::isValid(m_handle); }
  [[nodiscard]] uint32_t id() const { return m_handle.idx; }
  [[nodiscard]] bgfx::TextureHandle handle() const { return m_handle; }

  // load texture from file
  static std::shared_ptr<Texture> load2D(
      const std::filesystem::path& file, TextureType type = TextureType::IMAGE,
      uint64_t sampler = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);

private:
  // constructor used by load2D; Creates textures with existing GPU handles
  Texture(bgfx::TextureHandle h, uint16_t w, uint16_t hgt, TextureType k);

  // Define texture properties and GPU resource handle
  bgfx::TextureHandle m_handle{bgfx::kInvalidHandle};
  uint16_t m_width = 0;
  uint16_t m_height = 0;
  TextureType m_type = TextureType::EMPTY;
};

// Singleton, prevent duplicate texture loads
class TextureCache {
public:
  static TextureCache& instance();

  // Retrieve cached texture; loads texture if not found
  std::shared_ptr<Texture> get(const std::filesystem::path& file,
                                 TextureType type = TextureType::IMAGE);

  void clear();  // TODO: integrate with a resource manager

private:
  std::mutex m_mtx;
  std::unordered_map<std::string, std::shared_ptr<Texture>> m_map;
};

}  // namespace Gfx

#endif  // TEXTURE2D_H