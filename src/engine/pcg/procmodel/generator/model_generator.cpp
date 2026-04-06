#include "model_generator.h"

#include <random>

namespace ProcModel {

std::optional<ResolvedModel> ModelGenerator::Generate(
    const ModelGraph& graph, const ModelDescriptor& descriptor, uint64_t seed,
    int max_retries) {

  for (int attempt = 0; attempt < max_retries; ++attempt) {
    pcg32 rng(seed + attempt);

    std::unordered_set<std::string> selected_ids;
    std::vector<ResolvedDescriptor> resolved_descriptors;
    bool failed = false;

    // Collect root groups
    std::vector<const SelectionGroup*> pending;
    for (const auto& group : descriptor.selection_groups) {
      if (group.activated_by.empty()) {
        pending.push_back(&group);
      }
    }

    while (!pending.empty() && !failed) {
      const SelectionGroup* group = pending.back();
      pending.pop_back();

      // Filter parts by constraints
      std::vector<const PartDescriptor*> valid;
      for (const auto& part : group->parts) {
        if (IsValidSelection(part.id, selected_ids, descriptor.constraints)) {
          valid.push_back(&part);
        }
      }

      if (valid.empty()) {
        if (group->required) {
          failed = true;
          break;
        }
        continue;
      }

      const PartDescriptor* chosen = WeightedSelect(valid, rng);
      selected_ids.insert(chosen->id);

      // Look up the graph node for mesh references
      auto it = graph.node_lookup.find(chosen->id);
      if (it == graph.node_lookup.end()) {
        failed = true;
        break;
      }

      ModelGraphNode* node = it->second;

      ResolvedDescriptor resolved;
      resolved.descriptor_id = chosen->id;
      resolved.group_id = group->group_id;
      resolved.mesh_indices = node->mesh_indices;
      resolved.local_transform = node->local_transform;
      resolved.attach_to = group->attach_to;

      // Sample parameter ranges from annotations
      ApplyParameterRanges(resolved, *node, rng);

      if (group->attach_to.empty()) {
        resolved_descriptors.push_back(std::move(resolved));
      } else {
        for (const auto& attach_id : group->attach_to) {
          auto attach_it = graph.node_lookup.find(attach_id);
          if (attach_it == graph.node_lookup.end())
            continue;

          ResolvedDescriptor attached = resolved;
          attached.local_transform = attach_it->second->local_transform;
          attached.descriptor_id = resolved.descriptor_id + "_at_" + attach_id;
          resolved_descriptors.push_back(std::move(attached));
        }
      }
      // Activate dependent groups
      for (const auto& g : descriptor.selection_groups) {
        if (g.activated_by == chosen->id) {
          pending.push_back(&g);
        }
      }
    }

    if (failed)
      continue;

    // Apply parameter bindings across resolved descriptors
    ApplyParameterBindings(resolved_descriptors, descriptor.parameter_bindings);

    // Final constraint check
    if (ValidateConstraints(selected_ids, descriptor.constraints)) {
      ResolvedModel result;
      result.model_id = descriptor.model_id;
      result.seed = seed + attempt;
      result.descriptors = std::move(resolved_descriptors);
      result.model_scale =
          glm::vec3(1.0f);  // TODO: sample from descriptor scale range
      return result;
    }
  }

  return std::nullopt;
}

const PartDescriptor* ModelGenerator::WeightedSelect(
    const std::vector<const PartDescriptor*>& candidates, pcg32& rng) {
  float total_weight = 0.0f;
  for (const auto* c : candidates) {
    total_weight += c->weight;
  }

  std::uniform_real_distribution<float> dist(0.0f, total_weight);
  float roll = dist(rng);

  float cumulative = 0.0f;
  for (const auto* c : candidates) {
    cumulative += c->weight;
    if (roll <= cumulative) {
      return c;
    }
  }

  return candidates.back();
}

bool ModelGenerator::IsValidSelection(
    const std::string& part_id,
    const std::unordered_set<std::string>& selected_ids,
    const std::vector<ConstraintRule>& constraints) {
  for (const auto& rule : constraints) {
    if (rule.type == ConstraintRule::Type::EXCLUDES) {
      // If part_a is already selected, part_b is invalid (and vice versa)
      if (rule.part_a == part_id && selected_ids.count(rule.part_b) > 0)
        return false;
      if (rule.part_b == part_id && selected_ids.count(rule.part_a) > 0)
        return false;
    }
    if (rule.type == ConstraintRule::Type::REQUIRES) {
      // If this part requires another that hasn't been selected yet,
      // skip — it may be selected later. Only reject if the required
      // part was already excluded by group processing.
      // For now, REQUIRES is checked in ValidateConstraints post-generation.
    }
  }
  return true;
}

// Final matrix is composed in ModelInstantiator
void ModelGenerator::ApplyParameterRanges(ResolvedDescriptor& resolved,
                                          const ModelGraphNode& node,
                                          pcg32& rng) {
  for (const auto* range : node.parameter_ranges) {
    if (range->rotation_min && range->rotation_max) {
      std::uniform_real_distribution<float> dist_x(range->rotation_min->x,
                                                   range->rotation_max->x);
      std::uniform_real_distribution<float> dist_y(range->rotation_min->y,
                                                   range->rotation_max->y);
      std::uniform_real_distribution<float> dist_z(range->rotation_min->z,
                                                   range->rotation_max->z);

      resolved.applied_rotation =
          glm::vec3(dist_x(rng), dist_y(rng), dist_z(rng));
    }
  }
}

void ModelGenerator::ApplyParameterBindings(
    std::vector<ResolvedDescriptor>& descriptors,
    const std::vector<ParameterBinding>& bindings) {
  std::unordered_map<std::string, ResolvedDescriptor*> lookup;
  for (auto& desc : descriptors) {
    lookup[desc.descriptor_id] = &desc;
  }

  for (const auto& binding : bindings) {
    auto src_it = lookup.find(binding.source_part);
    auto tgt_it = lookup.find(binding.target_part);

    if (src_it == lookup.end() || tgt_it == lookup.end())
      continue;

    if (binding.source_param == "rotation" &&
        binding.target_param == "rotation") {
      tgt_it->second->applied_rotation =
          src_it->second->applied_rotation * binding.ratio;
    }
  }
}

bool ModelGenerator::ValidateConstraints(
    const std::unordered_set<std::string>& selected_ids,
    const std::vector<ConstraintRule>& constraints) {
  for (const auto& rule : constraints) {
    bool a_selected = selected_ids.count(rule.part_a) > 0;
    bool b_selected = selected_ids.count(rule.part_b) > 0;

    switch (rule.type) {
      case ConstraintRule::Type::EXCLUDES:
        if (a_selected && b_selected)
          return false;
        break;
      case ConstraintRule::Type::REQUIRES:
        if (a_selected && !b_selected)
          return false;
        if (b_selected && !a_selected)
          return false;
        break;
    }
  }
  return true;
}

}  // namespace ProcModel
