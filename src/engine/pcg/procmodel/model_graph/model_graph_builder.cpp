#include "model_graph_builder.h"

#include "engine/core/logger.h"
#include "engine/pcg/procmodel/model_graph/model_graph.h"

namespace ProcModel {

ModelGraphNode ModelGraphBuilder::ConvertNode(
    const Content::SceneNode& scene_node, const glm::mat4& parent_transform) {

  ModelGraphNode node;
  node.name = scene_node.name;
  node.local_transform = scene_node.local_transform;
  node.world_transform = parent_transform * scene_node.local_transform;
  node.mesh_indices = scene_node.mesh_indices;

  node.children.reserve(scene_node.children.size());
  for (const auto& child : scene_node.children) {
    node.children.push_back(ConvertNode(child, node.world_transform));
  }

  return node;
}

void ModelGraphBuilder::BuildLookup(
    ModelGraphNode& node,
    std::unordered_map<std::string, ModelGraphNode*>& lookup) {
  if (!node.name.empty()) {
    auto [it, inserted] = lookup.emplace(node.name, &node);
    if (!inserted) {
      Logger::getInstance().Log(
          LogLevel::Warning,
          "[ModelGraphBuilder] Duplicate node name: " + node.name);
    }
  }

  for (auto& child : node.children) {
    BuildLookup(child, lookup);
  }
}

ModelGraph ModelGraphBuilder::Build(const Content::SceneData& scene,
                                    const std::string& source_path) {
  ModelGraph graph;
  graph.source_path = source_path;
  graph.meshes = scene.meshes;
  graph.root = ConvertNode(scene.root, glm::mat4(1.0f));

  BuildLookup(graph.root, graph.node_lookup);

  return graph;
}

}  // namespace ProcModel
