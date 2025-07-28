#include "engine/resources/3d/mesh_data.h"

#include <unordered_map>
#include <vector>
#include <string>

#include "engine/core/logger.h"
#include "engine/formats/gltf_translator.h"

namespace Resources3D {

// has vector value
static std::unordered_map<std::string, std::vector<MeshData>> cache_;

const std::vector<MeshData>& Load(const std::string& path) {
  auto& mesh_cache = cache_;
  auto cache_it = mesh_cache.find(path);
  if (cache_it != mesh_cache.end())
    return cache_it->second;

  auto mesh_list = Formats::TranslateGLTF(path);  // switch on ext

  if (mesh_list.empty())
    Logger::getInstance().Log(LogLevel::Error,
                              "MeshCache failed to decode " + path);

  auto [emplace_it, was_inserted] = mesh_cache.emplace(path, std::move(mesh_list));
  return emplace_it->second;
}

}  // namespace Resources3D
