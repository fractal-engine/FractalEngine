#include "mesh_cache.h"

#include "engine/content/loaders/mesh_loader.h"
#include "engine/core/logger.h"

namespace Content {

MeshCache& MeshCache::Instance() {
  static MeshCache instance;
  return instance;
}

const std::vector<Geometry::MeshData>& MeshCache::Get(const std::string& path) {
  auto it = cache_.find(path);
  if (it != cache_.end())
    return it->second;

  auto mesh_data = MeshLoader::Load(path);
  if (mesh_data.empty()) {
    Logger::getInstance().Log(LogLevel::Warning,
                              "[MeshCache] No mesh_data loaded from: " + path);
    // Insert empty to avoid repeated load attempts
    cache_[path] = {};
    return cache_[path];
  }

  auto [emplace_it, _] = cache_.emplace(path, std::move(mesh_data));
  return emplace_it->second;
}

void MeshCache::Evict(const std::string& path) {
  cache_.erase(path);
}

void MeshCache::Clear() {
  cache_.clear();
}

}  // namespace Content