#ifndef TEXTURE_ASSET_H
#define TEXTURE_ASSET_H

#include <cstdint>
#include <filesystem>
#include <memory>

#include "engine/renderer/icons/icon_loader.h"
#include "engine/renderer/texture/texture2d.h"

namespace Assets {

class TextureAsset {
public:
  // placeholder, should match Gfx::TextureType later
  enum class ImportType {
    Image,
    Albedo,
    Normal,
    Roughness,
    Metallic,
    Occlusion,
    Emissive,
    Height
  };

  struct Meta {
    ImportType type{ImportType::Image};
    // TODO: width/height hints, sRGB flag, mip options, etc
  };

  explicit TextureAsset(std::filesystem::path file);
  ~TextureAsset() = default;

  // helpers
  uint32_t GpuHandle() const;  // bgfx handle idx OR fallback
  uint32_t Icon() const { return GpuHandle(); }

  const std::filesystem::path& Path() const { return file_path_; }
  void DrawInspectorUI();  // dummy function

  // placeholder hot-reload
  void Reload();

private:
  void LoadSync();  // immediate load via TextureCache

  std::filesystem::path file_path_;
  Meta meta_{};
  std::shared_ptr<Gfx::Texture> texture_;  // nullptr until loaded
};

}  // namespace Assets
#endif  // TEXTURE_ASSET_H