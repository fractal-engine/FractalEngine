#ifndef MODEL_GRAPH_NODE_H
#define MODEL_GRAPH_NODE_H

#include <glm/mat4x4.hpp>
#include <string>
#include <vector>

#include "engine/pcg/procmodel/descriptor/model_descriptor.h"

namespace ProcModel {

struct ModelGraphNode {
  std::string name;
  glm::mat4 local_transform;  // relative to node's parents
  glm::mat4 world_transform;
  std::vector<int> mesh_indices;  // empty if no geometry
  std::vector<ModelGraphNode> children;

  // Annotation slots
  std::string group_id;  // Empty if not part of a selection group
  const PartDescriptor* part = nullptr;  // Null if not a selectable part
  std::vector<const ParameterRange*> parameter_ranges;
  std::vector<const ParameterBinding*> outgoing_bindings;
  bool is_fixed = false;  // True if node always present
  bool is_attach_point = false;
  std::string attach_group_id;
};

}  // namespace ProcModel

#endif  // MODEL_GRAPH_NODE_H
