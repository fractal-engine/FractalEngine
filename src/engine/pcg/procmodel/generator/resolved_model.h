#ifndef RESOLVED_MODEL_H
#define RESOLVED_MODEL_H

#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <vector>

#include "engine/core/types/geometry_data.h"

namespace ProcModel {

struct InstanceGeometry {
  std::string descriptor_id;
  std::vector<Geometry::MeshData> mesh_data;
};

struct ResolvedDescriptor {
  std::string descriptor_id;
  std::string group_id;  // Which PartSlot this came from
  std::vector<int> mesh_indices;
  glm::mat4 local_transform;
  glm::vec3 applied_scale = glm::vec3(1.0f);
  glm::vec3 applied_rotation = glm::vec3(0.0f);

  std::string activator_id;  // empty for root descriptors
};

// Contains list of descriptors
struct InstanceModel {
  std::string model_id;  // Which ModelDescriptor produced this
  uint64_t seed;

  std::vector<ResolvedDescriptor> descriptors;
  glm::vec3 model_scale;  // Whole-model scale if descriptor defines a range
};
}  // namespace ProcModel

#endif  // RESOLVED_MODEL_H
