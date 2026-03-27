#include "descriptor_builder.h"

//
// DESCRIPTOR BUILDER
// Infers selection groups from naming conventions
//
namespace ProcModel {

std::string DescriptorBuilder::ExtractGroupPrefix(
    const std::string& node_name) {
  size_t last_underscore = node_name.rfind('_');
  if (last_underscore == std::string::npos || last_underscore == 0) {
    return "";
  }
  return node_name.substr(0, last_underscore + 1);
}

SelectionGroup* DescriptorBuilder::FindOrCreateGroup(
    std::unordered_map<std::string, SelectionGroup>& groups,
    const std::string& group_id, const std::string& activated_by) {
  auto it = groups.find(group_id);
  if (it == groups.end()) {
    SelectionGroup group;
    group.group_id = group_id;
    group.activated_by = activated_by;
    group.required = true;
    groups[group_id] = std::move(group);
  }
  return &groups[group_id];
}

void DescriptorBuilder::TraverseAndBuildGroups(
    const ModelGraphNode& node, const std::string& parent_part_id,
    std::unordered_map<std::string, SelectionGroup>& groups) {

  std::unordered_map<std::string, std::vector<const ModelGraphNode*>>
      prefix_buckets;

  for (const auto& child : node.children) {
    std::string prefix = ExtractGroupPrefix(child.name);
    if (!prefix.empty()) {
      prefix_buckets[prefix].push_back(&child);
    }
  }

  for (const auto& [prefix, nodes] : prefix_buckets) {
    if (nodes.size() < 2) {
      TraverseAndBuildGroups(*nodes[0], nodes[0]->name, groups);
      continue;
    }

    std::string group_id = parent_part_id + prefix;
    SelectionGroup* group = FindOrCreateGroup(groups, group_id, parent_part_id);

    for (const auto* n : nodes) {
      PartDescriptor part;
      part.id = n->name;
      part.name = n->name.substr(prefix.length());
      part.weight = 1.0f;
      group->parts.push_back(std::move(part));

      TraverseAndBuildGroups(*n, n->name, groups);
    }
  }
}

ModelDescriptor DescriptorBuilder::Build(const std::string& model_id,
                                         const std::string& asset_path,
                                         const ModelGraph& graph) {
  std::unordered_map<std::string, SelectionGroup> groups;

  TraverseAndBuildGroups(graph.root, "", groups);

  ModelDescriptor descriptor;
  descriptor.model_id = model_id;
  descriptor.model_name = model_id;
  descriptor.path = asset_path;

  for (auto& [key, group] : groups) {
    descriptor.selection_groups.push_back(std::move(group));
  }

  return descriptor;
}

}  // namespace ProcModel
