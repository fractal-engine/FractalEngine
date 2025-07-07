#ifndef ASSET_REGISTRY_H
#define ASSET_REGISTRY_H

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class EditorAsset;          // base class
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

class AssetRegistry {
public:                            
  static void Create();                // populate table
  static const AssetInfo& Fallback();  // always valid
  static const AssetInfo* FetchByPath(const std::filesystem::path& path);

private:
  template <typename TAsset>
  static void Register(AssetType type,
                       const std::vector<std::string>& extensions);

  static std::unordered_map<std::string, AssetInfo> registry_;
  static AssetInfo fallback_asset_;
};

#endif  // ASSET_REGISTRY_H
