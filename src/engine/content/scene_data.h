#ifndef SCENE_DATA_H
#define SCENE_DATA_H

#include <glm/mat4x4.hpp>
#include <string>
#include <vector>

#include "engine/core/types/geometry_data.h"

namespace Content {

struct SceneNode {
  std::string name;
  glm::mat4 local_transform;
  std::vector<int> mesh_indices;
  std::vector<SceneNode> children;
};

struct SceneData {
  std::vector<Geometry::MeshData> meshes;
  SceneNode root;
};

}  // namespace Content


#endif // SCENE_DATA_H
