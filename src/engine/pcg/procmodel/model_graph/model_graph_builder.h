#ifndef MODEL_GRAPH_BUILDER_H
#define MODEL_GRAPH_BUILDER_H

#include <string>

#include "engine/content/scene_data.h"
#include "model_graph.h"

namespace ProcModel {

class ModelGraphBuilder {
public:
  static ModelGraph Build(const Content::SceneData& scene,
                          const std::string& source_path);

private:
  static ModelGraphNode ConvertNode(const Content::SceneNode& scene_node,
                                    const glm::mat4& parent_transform);
  static void BuildLookup(
      ModelGraphNode& node,
      std::unordered_map<std::string, ModelGraphNode*>& lookup);
};

}  // namespace ProcModel

#endif  // MODEL_GRAPH_BUILDER_H
