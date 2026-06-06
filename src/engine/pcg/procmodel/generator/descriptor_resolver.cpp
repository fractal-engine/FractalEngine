#include "descriptor_resolver.h"

#include <unordered_set>

#include "engine/core/logger.h"

namespace ProcModel {

DescriptorResolver::ResolveResult DescriptorResolver::Resolve(
    ModelGraph& graph, const ModelDescriptor& descriptor) {
  ResolveResult result;

  bool groups_ok = MapSelectionGroups(graph, descriptor, result.errors);
  bool ranges_ok = MapTransformRanges(graph, descriptor, result.errors);
  bool bindings_ok = MapParameterBindings(graph, descriptor, result.errors);
  bool deforms_ok = MapDeformationRanges(graph, descriptor, result.errors);
  bool locators_ok = MapLocators(graph, descriptor, result.errors);

  for (const auto& [name, node_ptr] : graph.node_lookup) {
    if (node_ptr->group_ids.empty() && node_ptr->transform_ranges.empty() &&
        !node_ptr->is_fixed && !node_ptr->is_locator) {
      result.warnings.push_back("Node '" + name +
                                "' not referenced by any descriptor entry");
    }
  }

  result.success =
      groups_ok && ranges_ok && bindings_ok && deforms_ok && locators_ok;

  if (!result.success) {
    for (const auto& error : result.errors) {
      Logger::getInstance().Log(LogLevel::Error,
                                "[DescriptorResolver] " + error);
    }
  }
  for (const auto& warn : result.warnings) {
    Logger::getInstance().Log(LogLevel::Warning,
                              "[DescriptorResolver] " + warn);
  }

  return result;
}

bool DescriptorResolver::MapSelectionGroups(ModelGraph& graph,
                                            const ModelDescriptor& descriptor,
                                            std::vector<std::string>& errors) {
  bool all_ok = true;

  for (const auto& group : descriptor.selection_groups) {
    for (const auto& part : group.parts) {
      auto it = graph.node_lookup.find(part.id);
      if (it == graph.node_lookup.end()) {
        errors.push_back("Part '" + part.id + "' in group '" + group.group_id +
                         "' not found in model graph");
        all_ok = false;
        continue;
      }

      ModelGraphNode* node = it->second;

      // Reject only same-group duplicates
      // Allows reusable parts to be shared across groups
      for (const auto& existing : node->group_ids) {
        if (existing == group.group_id) {
          errors.push_back("Node '" + part.id + "' listed twice in group '" +
                           group.group_id + "'");
          all_ok = false;
          break;
        }
      }

      node->group_ids.push_back(group.group_id);
      node->parts.push_back(&part);
    }
  }
  return all_ok;
}

bool DescriptorResolver::MapTransformRanges(ModelGraph& graph,
                                            const ModelDescriptor& descriptor,
                                            std::vector<std::string>& errors) {
  bool all_ok = true;

  for (const auto& range : descriptor.transform_ranges) {
    auto it = graph.node_lookup.find(range.part_id);
    if (it == graph.node_lookup.end()) {
      errors.push_back("TransformRange references unknown part '" +
                       range.part_id + "'");
      all_ok = false;
      continue;
    }

    it->second->transform_ranges.push_back(&range);
  }

  return all_ok;
}

bool DescriptorResolver::MapDeformationRanges(
    ModelGraph& graph, const ModelDescriptor& descriptor,
    std::vector<std::string>& errors) {
  // Build set of known group IDs from the descriptor's selection groups.
  std::unordered_set<std::string> known_groups;
  for (const auto& group : descriptor.selection_groups) {
    known_groups.insert(group.group_id);
  }

  bool all_ok = true;
  for (const auto& range : descriptor.part_deformation_ranges) {
    // Empty group_id is the model-wide default, so no check needed
    if (range.group_id.empty())
      continue;

    if (known_groups.find(range.group_id) == known_groups.end()) {
      errors.push_back("DeformationRange references unknown group '" +
                       range.group_id + "'");
      all_ok = false;
    }
  }
  return all_ok;
}

bool DescriptorResolver::MapParameterBindings(
    ModelGraph& graph, const ModelDescriptor& descriptor,
    std::vector<std::string>& errors) {
  bool all_ok = true;

  for (const auto& binding : descriptor.parameter_bindings) {
    auto src_it = graph.node_lookup.find(binding.source_part);
    if (src_it == graph.node_lookup.end()) {
      errors.push_back("ParameterBinding source '" + binding.source_part +
                       "' not found in model graph");
      all_ok = false;
      continue;
    }

    auto tgt_it = graph.node_lookup.find(binding.target_part);
    if (tgt_it == graph.node_lookup.end()) {
      errors.push_back("ParameterBinding target '" + binding.target_part +
                       "' not found in model graph");
      all_ok = false;
      continue;
    }

    src_it->second->outgoing_bindings.push_back(&binding);
  }

  return all_ok;
}

// Resolves locators, validates them and marks them on the graph
bool DescriptorResolver::MapLocators(ModelGraph& graph,
                                     const ModelDescriptor& descriptor,
                                     std::vector<std::string>& errors) {
  bool all_ok = true;

  for (const auto& group : descriptor.selection_groups) {
    if (!group.locators)
      continue;

    for (const auto& locator_id : *group.locators) {
      auto sock_it = graph.node_lookup.find(locator_id);
      if (sock_it == graph.node_lookup.end()) {
        errors.push_back("Group '" + group.group_id +
                         "' references unknown locator '" + locator_id + "'");
        all_ok = false;
        continue;
      }
      if (!sock_it->second->mesh_indices.empty()) {
        errors.push_back("Group '" + group.group_id + "' lists '" + locator_id +
                         "' as a locator but that node has mesh geometry");
        all_ok = false;
        continue;
      }
      sock_it->second->is_locator = true;
    }
  }

  for (const auto& group : descriptor.selection_groups) {
    if (!group.locators || group.parent.empty())
      continue;

    // Build a set of nodes reachable as descendants of any parent part
    std::unordered_set<std::string> reachable;
    std::function<void(const ModelGraphNode&)> collect =
        [&](const ModelGraphNode& n) {
          reachable.insert(n.name);
          for (const auto& c : n.children)
            collect(c);
        };
    for (const auto& parent_id : group.parent) {
      auto p_it = graph.node_lookup.find(parent_id);
      if (p_it != graph.node_lookup.end()) {
        collect(*p_it->second);
      }
    }

    Logger::getInstance().Log(LogLevel::Debug,
                              "[DescriptorResolver] Group '" + group.group_id +
                                  "' reachable nodes (" +
                                  std::to_string(reachable.size()) + "):");
    for (const auto& r : reachable) {
      Logger::getInstance().Log(
          LogLevel::Debug, "[DescriptorResolver]   reachable: '" + r + "'");
    }

    for (const auto& locator_id : *group.locators) {
      if (reachable.find(locator_id) == reachable.end()) {
        errors.push_back("Group '" + group.group_id + "' lists locator '" +
                         locator_id +
                         "' which is not a descendant of any of "
                         "its parent parts");
        all_ok = false;
      }
    }
  }

  return all_ok;
}

}  // namespace ProcModel
