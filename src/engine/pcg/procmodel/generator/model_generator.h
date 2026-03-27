#ifndef MODEL_GENERATOR_H
#define MODEL_GENERATOR_H

#include <cstdint>
#include <optional>
#include <pcg_random.hpp>
#include <string>
#include <unordered_set>
#include <vector>

#include "engine/pcg/procmodel/descriptor/model_descriptor.h"
#include "engine/pcg/procmodel/generator/resolved_model.h"
#include "engine/pcg/procmodel/model_graph/model_graph.h"

namespace ProcModel {

class ModelGenerator {
public:
  static std::optional<ResolvedModel> Generate(
      const ModelGraph& graph, const ModelDescriptor& descriptor, uint64_t seed,
      int max_retries = 10);

private:
  static const PartDescriptor* WeightedSelect(
      const std::vector<const PartDescriptor*>& candidates, pcg32& rng);

  static bool IsValidSelection(
      const std::string& part_id,
      const std::unordered_set<std::string>& selected_ids,
      const std::vector<ConstraintRule>& constraints);

  static void ApplyParameterRanges(ResolvedDescriptor& resolved,
                                   const ModelGraphNode& node, pcg32& rng);

  static void ApplyParameterBindings(
      std::vector<ResolvedDescriptor>& descriptors,
      const std::vector<ParameterBinding>& bindings);

  static bool ValidateConstraints(
      const std::unordered_set<std::string>& selected_ids,
      const std::vector<ConstraintRule>& constraints);
};

}  // namespace ProcModel

#endif
