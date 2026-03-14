#ifndef MESH_LOADER_H
#define MESH_LOADER_H

#include <string>
#include <vector>

#include "engine/core/types/geometry_data.h"

namespace Content {

class MeshLoader {
public:
  // Load from file (dispatch based on extension)
  static std::vector<Geometry::MeshData> Load(const std::string& path);

  static bool IsSupported(const std::string& path);
};

}  // namespace Content

#endif  // MESH_LOADER_H