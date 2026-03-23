#ifndef MODEL_GRAPH_NODE_H
#define MODEL_GRAPH_NODE_H

#include <glm/mat4x4.hpp>
#include <string>
#include <vector>

namespace ProcModel {

struct ModelGraphNode {
  std::string name;
  glm::mat4 local_transform;
  std::vector<int> mesh_indices;  // empty if no geometry
  std::vector<ModelGraphNode> children;
};

}  // namespace ProcModel

#endif // MODEL_GRAPH_NODE_H
