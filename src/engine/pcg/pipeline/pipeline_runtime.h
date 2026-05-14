#ifndef PCG_PIPELINE_RUNTIME_H
#define PCG_PIPELINE_RUNTIME_H

#include <memory>
#include <string>

#include "engine/pcg/pipeline/operation_context.h"
#include "engine/pcg/pipeline/operation_data.h"
#include "engine/pcg/pipeline/operation_registry.h"

namespace PCG {

// Runtime form of one operation in a compiled pipeline.
// Handler is captured at compile time so the hot path doesn't do name lookup.
struct CompiledOperation {
  std::string kind;  // ? kept for diagnostics / telemetry
  std::shared_ptr<OperationData> data;
  OperationRegistry::Handler handler;
};

// Compiled pipeline interface. Concrete topologies (linear, graph) implement
// Run() by deciding the order and conditions under which CompiledOperations
// fire.
class CompiledPipeline {
public:
  virtual ~CompiledPipeline() = default;
  virtual void Run(OperationContext& ctx) const = 0;
};

}  // namespace PCG

#endif  // PCG_PIPELINE_RUNTIME_H