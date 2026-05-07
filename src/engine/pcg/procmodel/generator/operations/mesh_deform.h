#ifndef MESH_DEFORM_H
#define MESH_DEFORM_H

#include <glm/vec3.hpp>
#include <optional>

#include "engine/pcg/procmodel/generator/model_operation_data.h"

namespace ProcModel {

class ModelOperationRegistry;

// MESH DEFORM
// Per-operation parameters
//
// Per-instance affine jitter applied on top of any TransformRange sampling
// done during core generation. Writes into ResolvedDescriptor's
// applied_rotation and applied_scale fields.
//
// parent_correlation:
//   0.0 = independent jitter per instance
//   1.0 = child fully inherits parent's jitter (siblings still independent)
struct MeshDeformData : public ModelOperationData {
  std::optional<glm::vec3> rotation_jitter_min;
  std::optional<glm::vec3> rotation_jitter_max;
  std::optional<glm::vec3> scale_jitter_min;
  std::optional<glm::vec3> scale_jitter_max;
  float parent_correlation = 0.0f;
};

void RegisterMeshDeformOperation(ModelOperationRegistry& registry);

}  // namespace ProcModel

#endif  // MESH_DEFORM_H