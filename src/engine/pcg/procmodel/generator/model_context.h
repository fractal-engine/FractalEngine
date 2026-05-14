#ifndef PROCMODEL_MODEL_CONTEXT_H
#define PROCMODEL_MODEL_CONTEXT_H

#include <pcg_random.hpp>

#include "engine/core/types/geometry_data.h"

#include "engine/pcg/pipeline/operation_context.h"

#include "engine/pcg/procmodel/model_graph/model_graph.h"

namespace ProcModel {

struct ModelDescriptor;
struct ResolvedModel;

struct InstanceGeometry {
  std::string descriptor_id;
  std::vector<Geometry::MeshData> mesh_data;
};

// Concrete operation context for ProcModel pipelines.
// References the descriptor (read-only inputs), the resolved model (mutable
// state operations transform), and the RNG.
struct ModelContext : public PCG::OperationContext {
  const ModelDescriptor& descriptor;
  const ModelGraph& graph;
  ResolvedModel& model;
  pcg32& rng;

  // Per-descriptor geometry produced by vertex deformation operations.
  std::vector<InstanceGeometry> instance_geometry;

  ModelContext(const ModelDescriptor& d, const ModelGraph& g, ResolvedModel& m,
               pcg32& r)
      : descriptor(d), graph(g), model(m), rng(r) {}
};

}  // namespace ProcModel

#endif  // PROCMODEL_MODEL_CONTEXT_H