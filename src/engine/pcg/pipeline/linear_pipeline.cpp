#include "linear_pipeline.h"

#include "engine/pcg/pipeline/pipeline_descriptor.h"

namespace PCG {

void LinearPipeline::Run(OperationContext& ctx) const {
  for (const auto& op : ops_) {
    op.handler(*op.data, ctx);
  }
}

std::optional<LinearPipeline> LinearPipeline::Compile(
    const PipelineDescriptor& source, const OperationRegistry& registry,
    std::vector<std::string>& errors) {

  LinearPipeline pipeline;
  pipeline.ops_.reserve(source.entries.size());

  for (size_t i = 0; i < source.entries.size(); ++i) {
    const auto& entry = source.entries[i];

    if (!registry.IsRegistered(entry.kind)) {
      errors.push_back("Pipeline entry " + std::to_string(i) +
                       ": unknown operation kind '" + entry.kind + "'");
      continue;
    }

    auto data = registry.Parse(entry.kind, entry.params);
    if (!data) {
      errors.push_back("Pipeline entry " + std::to_string(i) +
                       ": parser failed for kind '" + entry.kind + "'");
      continue;
    }

    auto handler = registry.GetHandler(entry.kind);
    if (!handler) {
      errors.push_back("Pipeline entry " + std::to_string(i) +
                       ": no handler bound for kind '" + entry.kind + "'");
      continue;
    }

    CompiledOperation compiled;
    compiled.kind = entry.kind;
    compiled.data = std::move(data);
    compiled.handler = std::move(handler);
    pipeline.ops_.push_back(std::move(compiled));
  }

  if (!errors.empty()) {
    return std::nullopt;
  }

  return pipeline;
}

}  // namespace PCG