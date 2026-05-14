#ifndef PCG_LINEAR_PIPELINE_H
#define PCG_LINEAR_PIPELINE_H

#include <optional>
#include <string>
#include <vector>

#include "engine/pcg/pipeline/operation_registry.h"
#include "engine/pcg/pipeline/pipeline_runtime.h"

namespace PCG {

struct PipelineDescriptor;

// Linear topology: operations execute in source order, one after another.
// Each operation reads the same context, mutates it, and the next operation
// sees those mutations. No branching, no fan-in, no fan-out.
class LinearPipeline final : public CompiledPipeline {
public:
  LinearPipeline() = default;

  void Run(OperationContext& ctx) const override;

  // Compile a source-form pipeline into a LinearPipeline.
  // Returns std::nullopt if any entry fails to parse or has an unknown kind.
  // All errors are gathered before returning, not just the first.
  static std::optional<LinearPipeline> Compile(
      const PipelineDescriptor& source, const OperationRegistry& registry,
      std::vector<std::string>& errors);

  bool IsEmpty() const { return ops_.empty(); }
  size_t Size() const { return ops_.size(); }

private:
  std::vector<CompiledOperation> ops_;
};

}  // namespace PCG

#endif  // PCG_LINEAR_PIPELINE_H