#include "descriptor_resolver.h"

#include "engine/core/logger.h"

namespace ProcModel {

DescriptorResolver::ResolveResult DescriptorResolver::Resolve(
    ModelGraph& graph, const ModelDescriptor& descriptor) {
  ResolveResult result;

  bool groups_ok = MapSelectionGroups(graph, descriptor, result.errors);
  bool ranges_ok = MapParameterRanges(graph, descriptor, result.errors);
  bool bindings_ok = MapParameterBindings(graph, descriptor, result.errors);

  for (const auto& [name, node_ptr] : graph.node_lookup) {
    if (node_ptr->group_id.empty() && node_ptr->parameter_ranges.empty() &&
        !node_ptr->is_fixed && !node_ptr->is_attach_point) {
      result.warnings.push_back("Node '" + name +
                                "' not referenced by any descriptor entry");
    }
  }

  result.success = groups_ok && ranges_ok && bindings_ok;

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

      // Check for duplicate assignment
      if (!node->group_id.empty()) {
        errors.push_back("Node '" + part.id + "' already assigned to group '" +
                         node->group_id + "', cannot assign to '" +
                         group.group_id + "'");
        all_ok = false;
        continue;
      }

      node->group_id = group.group_id;
      node->part = &part;
    }

    // Check for attachment points
    for (const auto& attach_name : group.attach_to) {
      auto att_it = graph.node_lookup.find(attach_name);
      if (att_it == graph.node_lookup.end()) {
        errors.push_back("Attach point '" + attach_name + "' in group '" +
                         group.group_id + "' not found in model graph");
        all_ok = false;
        continue;
      }
      att_it->second->is_attach_point = true;
      att_it->second->attach_group_id = group.group_id;
    }
  }
  return all_ok;
}

bool DescriptorResolver::MapParameterRanges(ModelGraph& graph,
                                            const ModelDescriptor& descriptor,
                                            std::vector<std::string>& errors) {
  bool all_ok = true;

  for (const auto& range : descriptor.parameter_ranges) {
    auto it = graph.node_lookup.find(range.part_id);
    if (it == graph.node_lookup.end()) {
      errors.push_back("ParameterRange references unknown part '" +
                       range.part_id + "'");
      all_ok = false;
      continue;
    }

    it->second->parameter_ranges.push_back(&range);
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

}  // namespace ProcModel
