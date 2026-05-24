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

// Find or create a selection group by ID, and optionally record a parent ID.
// - If the group is new and parent is non-empty, parent is stored.
// - If the group exists, parent is appended only if not already present.
// - Root groups should have an empty parent list (no empty-string entry).
SelectionGroup* DescriptorBuilder::FindOrCreateGroup(
    std::unordered_map<std::string, SelectionGroup>& groups,
    const std::string& group_id, const std::string& parent) {
  auto it = groups.find(group_id);
  if (it == groups.end()) {
    SelectionGroup group;
    group.group_id = group_id;
    if (!parent.empty()) {
      group.parent.push_back(parent);
    }
    group.required = true;
    groups[group_id] = std::move(group);
  } else if (!parent.empty()) {
    // Append parent if not already listed
    auto& existing_parents = it->second.parent;
    if (std::find(existing_parents.begin(), existing_parents.end(), parent) ==
        existing_parents.end()) {
      existing_parents.push_back(parent);
    }
  }
  return &groups[group_id];
}

void DescriptorBuilder::TraverseAndBuildGroups(
    const ModelGraphNode& node, const std::string& parent_part_id,
    std::unordered_map<std::string, SelectionGroup>& groups) {

  std::unordered_map<std::string, std::vector<const ModelGraphNode*>>
      prefix_buckets;

  for (const auto& child : node.children) {
    // Skip nodes that look like sockets (empty mesh nodes are locators,
    // not parts). The builder only infers parts; sockets are handled by
    // the resolver from the GLTF graph directly.
    if (child.mesh_indices.empty()) {
      continue;
    }

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
