#ifndef RESOLVED_MODEL_H
#define RESOLVED_MODEL_H

#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <vector>

namespace ProcModel {

struct ResolvedDescriptor {
  std::string descriptor_id;
  std::string group_id;  // Which PartSlot this came from
  std::vector<int> mesh_indices;
  glm::mat4 local_transform;
  glm::vec3 applied_scale;
  glm::vec3 applied_rotation;
  std::vector<std::string> attach_to;
};

// Contains list of descriptors
struct ResolvedModel {
  std::string model_id;  // Which ModelDescriptor produced this
  uint64_t seed;

  std::vector<ResolvedDescriptor> descriptors;
  glm::vec3 model_scale;  // Whole-model scale if descriptor defines a range
};

}  // namespace ProcModel

#endif  // RESOLVED_MODEL_H
