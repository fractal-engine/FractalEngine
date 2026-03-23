#include "descriptor_builder.h"

SelectionGroup* FindOrCreateGroup(
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

std::string ExtractGroupPrefix(const std::string& node_name) {
  // "_HEAD_A" -> "_HEAD_"
  // Find last underscore, take everything before it (inclusive)
  size_t last_underscore = node_name.rfind('_');
  if (last_underscore == std::string::npos || last_underscore == 0) {
    return "";
  }
  return node_name.substr(0, last_underscore + 1);
}

void TraverseAndBuildGroups(
    const ModelGraphNode& node, const std::string& parent_candidate_id,
    std::unordered_map<std::string, SelectionGroup>& groups) {
  // Group children by prefix
  std::unordered_map<std::string, std::vector<const ModelGraphNode*>>
      prefix_buckets;

  for (const auto& child : node.children) {
    std::string prefix = ExtractGroupPrefix(child.name);
    if (!prefix.empty()) {
      prefix_buckets[prefix].push_back(&child);
    }
  }

  // Each bucket with >1 node becomes a selection group
  for (const auto& [prefix, nodes] : prefix_buckets) {
    if (nodes.size() < 2) {
      // Single node, not a selection group—just traverse its children
      TraverseAndBuildGroups(*nodes[0], nodes[0]->name, groups);
      continue;
    }

    // Create group
    std::string group_id = parent_candidate_id + prefix;
    SelectionGroup* group =
        FindOrCreateGroup(groups, group_id, parent_candidate_id);

    for (const auto* n : nodes) {
      PartCandidate candidate;
      candidate.id = n->name;
      candidate.name =
          n->name.substr(prefix.length());  // Strip prefix for display
      candidate.weight = 1.0f;
      group->candidates.push_back(candidate);

      // Recurse into this candidate's children
      TraverseAndBuildGroups(*n, n->name, groups);
    }
  }
}

ModelDescriptor BuildDescriptorFromGraph(const std::string& archetype_id,
                                         const std::string& asset_path,
                                         const ModelGraph& graph) {
  std::unordered_map<std::string, SelectionGroup> groups;

  // Start from root, no parent
  TraverseAndBuildGroups(graph.root, "", groups);

  ModelDescriptor descriptor;
  descriptor.archetype_id = archetype_id;
  descriptor.archetype_name = archetype_id;
  descriptor.asset_path = asset_path;

  for (auto& [key, group] : groups) {
    descriptor.selection_groups.push_back(std::move(group));
  }

  return descriptor;
}