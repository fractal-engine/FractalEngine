#ifndef MESH_DATA_H
#define MESH_DATA_H

#include <cstdint>
#include <string>
#include <vector>

namespace Resources3D {

struct MeshData {
  std::vector<float> positions_;
  std::vector<float> normals_;
  std::vector<uint32_t> indices_;  // 32-bit
  std::string material_name_ = "default";
};

// TODO: make it thread-safe, should use TaskFlags implementation
const std::vector<MeshData>& Load(const std::string& path);

}  // namespace Resources3D

#endif  // MESH_DATA_H