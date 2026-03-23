#include "model_generator.h"

std::optional<ResolvedModel> Generate(const ModelDescriptor& descriptor,
                                      uint64_t seed, int max_retries = 10) {
  for (int attempt = 0; attempt < max_retries; ++attempt) {
    pcg32 rng(seed + attempt);

    std::unordered_set<std::string> selected_ids;
    std::vector<ResolvedPart> parts;

    // Find root groups (no activation dependency)
    std::vector<const SelectionGroup*> pending;
    for (const auto& group : descriptor.selection_groups) {
      if (group.activated_by.empty()) {
        pending.push_back(&group);
      }
    }

    // Process until no more groups to evaluate
    while (!pending.empty()) {
      const SelectionGroup* group = pending.back();
      pending.pop_back();

      // Filter candidates by constraints
      std::vector<const PartCandidate*> valid;
      for (const auto& candidate : group->candidates) {
        if (IsValidSelection(candidate.id, selected_ids,
                             descriptor.constraints)) {
          valid.push_back(&candidate);
        }
      }

      if (valid.empty()) {
        if (group->required) {
          goto retry;  // No valid choice, retry with new seed
        }
        continue;
      }

      // Weighted selection
      const PartCandidate* chosen = WeightedSelect(valid, rng);
      selected_ids.insert(chosen->id);

      ResolvedPart part;
      part.part_id = chosen->id;
      part.group_id = group->group_id;
      // Transform sampling happens later
      parts.push_back(part);

      // Activate child groups
      for (const auto& g : descriptor.selection_groups) {
        if (g.activated_by == chosen->id) {
          pending.push_back(&g);
        }
      }
    }

    // Sample parameters for selected parts
    for (auto& part : parts) {
      ApplyParameterRanges(part, selected_ids, descriptor.parameter_ranges,
                           rng);
    }

    // Apply linked variations
    ApplyLinkedVariations(parts, descriptor.linked_variations);

    // Final constraint validation
    if (ValidateAllConstraints(selected_ids, descriptor.constraints)) {
      ResolvedModel result;
      result.archetype_id = descriptor.archetype_id;
      result.seed = seed + attempt;
      result.instance_id = GenerateId(result.archetype_id, result.seed);
      result.parts = std::move(parts);
      return result;
    }

  retry:;
  }

  return std::nullopt;
}