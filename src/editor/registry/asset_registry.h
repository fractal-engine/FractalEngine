#ifndef ASSET_REGISTRY_H
#define ASSET_REGISTRY_H

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class EditorAsset;
class TextureAsset;
using AssetSid = uint64_t;  // incremental, per-session ID

// Asset categories
enum class AssetType {
  kFallback = 0,
  kTexture,
  // kFont,
  // kMesh ,
  // TODO: add more asset types as needed
};

// Stored information block for each file-extension
struct AssetInfo {
  AssetType type_{};
  std::function<std::shared_ptr<EditorAsset>()> create_instance_;
  std::function<void(AssetSid /*sid*/)> inspect_;
};

namespace AssetRegistry {

// Global registry and fallback asset
extern std::unordered_map<std::string, std::shared_ptr<AssetInfo>> g_registry_;
extern std::shared_ptr<AssetInfo> fallback_asset_;

// Create asset info for a specific type
template <typename TAsset>
std::shared_ptr<AssetInfo> CreateAssetInfo(AssetType type) {
  static_assert(std::is_base_of_v<EditorAsset, TAsset>,
                "Only subclasses of EditorAsset can be registered");

  auto info = std::make_shared<AssetInfo>();
  info->type_ = type;
  info->create_instance_ = [] {
    return std::make_shared<TAsset>();
  };
  info->inspect_ = [](AssetSid sid) {
    // Hand the SID over to your inspector system
    EditorLayer::Get()->OpenAssetInspector(sid);
  };

  return info;
}

// Register an asset type with multiple extensions
template <typename TAsset>
void RegisterAsset(AssetType type, const std::vector<std::string>& extensions) {
  auto asset_info = CreateAssetInfo<TAsset>(type);

  for (const std::string& ext : extensions) {
    g_registry_[ext] = asset_info;
  }
}

void Create();                                 // Populate table

const std::shared_ptr<AssetInfo>& Fallback();  // Always valid

std::shared_ptr<AssetInfo> FetchByPath(const std::filesystem::path& path);
const std::unordered_map<std::string, std::shared_ptr<AssetInfo>>& Get();

}  // namespace AssetRegistry

#endif  // ASSET_REGISTRY_H
