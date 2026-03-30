#ifndef MODEL_INSTANTIATOR_H
#define MODEL_INSTANTIATOR_H

#include <vector>

#include "engine/ecs/world.h"

#include "engine/pcg/procmodel/generator/resolved_model.h"
#include "engine/pcg/procmodel/model_graph/model_graph.h"

namespace ProcModel {

class ModelInstantiator {
public:
  struct InstantiateResult {
    Entity root = entt::null;
    std::vector<Entity> part_entities;
  };

  static InstantiateResult Instantiate(const ResolvedModel& resolved,
                                       const ModelGraph& graph,
                                       Entity parent = entt::null);

private:
  static Entity CreatePartEntity(const ResolvedDescriptor& descriptor,
                                 const ModelGraph& graph, Entity parent);
};

}  // namespace ProcModel

#endif  // MODEL_INSTANTIATOR_H
