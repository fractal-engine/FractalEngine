#ifndef MESH_DEFORM_H
#define MESH_DEFORM_H

#include <glm/vec3.hpp>
#include <optional>

#include "engine/pcg/pipeline/operation_data.h"

namespace PCG {
class OperationRegistry;
}

namespace ProcModel {

// MESH DEFORM
// Per-operation parameters
//
// Per-instance affine jitter applied on top of any TransformRange sampling
// done during core generation. Writes into ResolvedDescriptor's
// applied_rotation and applied_scale fields.
struct MeshDeformData : public PCG::OperationData {
  std::optional<glm::vec3> rotation_jitter_min;
  std::optional<glm::vec3> rotation_jitter_max;
  std::optional<glm::vec3> scale_jitter_min;
  std::optional<glm::vec3> scale_jitter_max;
};

void RegisterMeshDeformOperation(PCG::OperationRegistry& registry);

}  // namespace ProcModel

#endif  // MESH_DEFORM_H