#include "asset_registry.h"
#include "editor/panels/inspector_panel.h"
#include "editor/systems/texture_asset.h"
#include "engine/core/logger.h"

namespace AssetRegistry {

// Global registry for all assets
std::unordered_map<std::string, std::shared_ptr<AssetInfo>> g_registry_;

// Asset info for fallback asset
std::shared_ptr<AssetInfo> fallback_asset_;

// Returns fallback asset
const std::shared_ptr<AssetInfo>& Fallback() {
  return fallback_asset_;
}

// Returns full registry of asset types
const std::unordered_map<std::string, std::shared_ptr<AssetInfo>>& Get() {
  return g_registry_;
}

// Finds asset handler based on file extension
std::shared_ptr<AssetInfo> FetchByPath(const std::filesystem::path& path) {
  std::string ext = path.extension().string();  // ".png"
  if (!ext.empty() && ext.front() == '.')
    ext.erase(ext.begin());

  auto it = g_registry_.find(ext);
  if (it != g_registry_.end())
    return it->second;

  if (ext != "meta")  // .meta should not open anything
    return fallback_asset_;

  return nullptr;
}

// Init asset registry with all supported asset types
void Create() {
  // 1. Create fallback asset (used when no matching extension is found)
  fallback_asset_ = CreateAssetInfo<EditorAsset>(AssetType::kFallback);

  // 2. Register every asset type you actually have
  RegisterAsset<TextureAsset>(
      AssetType::kTexture, {"png", "jpg", "jpeg", "bmp", "tga", "gif", "hdr"});

  // TODO: add more asset types here
}

}  // namespace AssetRegistry
