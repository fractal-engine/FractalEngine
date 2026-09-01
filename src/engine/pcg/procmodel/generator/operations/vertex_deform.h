#ifndef VERTEX_DEFORM_H
#define VERTEX_DEFORM_H

#include <optional>

#include "engine/pcg/pipeline/operation_data.h"
#include "engine/pcg/pipeline/operation_registry.h"

namespace ProcModel {

// Barr 1984 — global axis selector for which local axis the deformation
// acts along. The geometric formulas assume y-axis (twist about z, bend
// along y-centerline); the implementation rotates into that basis first.
enum class DeformAxis { X, Y, Z };

// PER-PART OPERATION
// Reads parameter ranges from each ResolvedDescriptor's part annotations
// (TransformRange), samples them with the context RNG, and writes the
// deformed result into ctx.instance_geometry
struct PartDeformData : public PCG::OperationData {
  bool apply_taper = false;
  bool apply_twist = false;
  bool apply_bend = false;
  bool apply_noise = false;

  DeformAxis taper_axis = DeformAxis::Y;
  DeformAxis twist_axis = DeformAxis::Y;
  DeformAxis bend_axis = DeformAxis::Y;

  // Noise spatial frequency (cycles per source-mesh unit)
  float noise_frequency = 1.0f;
};

// INSTANCE-WIDE OPERATION
// Runs after all per-part operations. Reads current ctx.instance_geometry
// and applies a single model-wide deformation
//
// Different semantics: the bend/twist axis is computed from
// the whole model's AABB, not per-part.
struct InstanceDeformData : public PCG::OperationData {
  bool apply_taper = false;
  bool apply_twist = false;
  bool apply_bend = false;

  DeformAxis taper_axis = DeformAxis::Y;
  DeformAxis twist_axis = DeformAxis::Y;
  DeformAxis bend_axis = DeformAxis::Y;
};

void RegisterVertexDeformOperation(PCG::OperationRegistry& registry);

}  // namespace ProcModel

#endif  // VERTEX_DEFORM_H