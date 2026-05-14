#include "duplicate_instances.h"

#include <nlohmann/json.hpp>
#include <random>

#include "engine/pcg/pipeline/operation_context.h"
#include "engine/pcg/pipeline/operation_registry.h"
#include "engine/pcg/procmodel/descriptor/model_descriptor.h"
#include "engine/pcg/procmodel/generator/model_context.h"
#include "engine/pcg/procmodel/generator/resolved_model.h"
namespace ProcModel {

static std::shared_ptr<PCG::OperationData> ParseDuplicateInstances(
    const nlohmann::json& params) {
  auto data = std::make_shared<DuplicateInstancesData>();

  data->target_group_id = params.value("target_group_id", std::string(""));
  data->min_count = params.value("min_count", 1u);
  data->max_count = params.value("max_count", 1u);

  if (data->min_count > data->max_count) {
    return nullptr;  // invalid range
  }
  if (data->min_count < 1) {
    return nullptr;  // count of 0 would erase the original
  }

  return data;
}

static void ApplyDuplicateInstances(const PCG::OperationData& base_data,
                                    PCG::OperationContext& base_ctx) {

  const auto& data = PCG::OperationCast<DuplicateInstancesData>(base_data);
  auto& ctx = PCG::OperationCast<ModelContext>(base_ctx);

  // Snapshot the descriptors we want to duplicate. We don't iterate the live
  // vector while pushing to it.
  std::vector<size_t> source_indices;
  for (size_t i = 0; i < ctx.model.descriptors.size(); ++i) {
    const auto& d = ctx.model.descriptors[i];
    if (data.target_group_id.empty() || d.group_id == data.target_group_id) {
      source_indices.push_back(i);
    }
  }

  std::uniform_int_distribution<std::uint32_t> dist(data.min_count,
                                                    data.max_count);

  for (size_t src_idx : source_indices) {
    const std::uint32_t total_count = dist(ctx.rng);

    // total_count includes the original, so we add (total_count - 1) copies.
    for (std::uint32_t copy_n = 1; copy_n < total_count; ++copy_n) {
      // Re-fetch on every push because the vector may reallocate.
      ResolvedDescriptor copy = ctx.model.descriptors[src_idx];
      copy.descriptor_id = ctx.model.descriptors[src_idx].descriptor_id +
                           "_dup_" + std::to_string(copy_n);
      ctx.model.descriptors.push_back(std::move(copy));
    }
  }
}

void RegisterDuplicateInstancesOperation(PCG::OperationRegistry& registry) {
  registry.Register("duplicate_instances", &ParseDuplicateInstances,
                    &ApplyDuplicateInstances);
}

}  // namespace ProcModel