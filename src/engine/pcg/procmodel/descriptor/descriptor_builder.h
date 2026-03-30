#ifndef DESCRIPTOR_BUILDER_H
#define DESCRIPTOR_BUILDER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "engine/pcg/procmodel/descriptor/model_descriptor.h"
#include "engine/pcg/procmodel/model_graph/model_graph.h"

namespace ProcModel {

class DescriptorBuilder {
public:
  static ModelDescriptor Build(const std::string& model_id,
                               const std::string& asset_path,
                               const ModelGraph& graph);

private:
  static std::string ExtractGroupPrefix(const std::string& node_name);

  static SelectionGroup* FindOrCreateGroup(
      std::unordered_map<std::string, SelectionGroup>& groups,
      const std::string& group_id, const std::string& activated_by);

  static void TraverseAndBuildGroups(
      const ModelGraphNode& node, const std::string& parent_part_id,
      std::unordered_map<std::string, SelectionGroup>& groups);
};

}  // namespace ProcModel

#endif  // DESCRIPTOR_BUILDER_H
