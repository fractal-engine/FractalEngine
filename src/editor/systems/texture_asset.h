#ifndef TEXTURE_ASSET_H
#define TEXTURE_ASSET_H

#include <cstdint>
#include <filesystem>
#include <memory>

#include "editor/systems/editor_asset.h"
#include "engine/renderer/icons/icon_loader.h"
#include "engine/renderer/texture/texture2d.h"

class TextureAsset : public EditorAsset {
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

  TextureAsset();
  ~TextureAsset() override;

  explicit TextureAsset(std::filesystem::path file); // constructor

  void OnDefaultLoad(const std::filesystem::path& meta_path) override {}
  void OnUnload() override {}
  void OnReload() override {}
  void DrawInspectorUI() override;
  uint32_t GetIcon() const override { return GpuHandle(); }

  // helpers
  uint32_t GpuHandle() const;  // bgfx handle idx OR fallback
  void Reload();               // placeholder hot-reload

private:
  void LoadSync();  // immediate load via TextureCache

  std::filesystem::path file_path_;
  Meta meta_{};
  std::shared_ptr<Gfx::Texture> texture_;  // nullptr until loaded
};

#endif  // TEXTURE_ASSET_H