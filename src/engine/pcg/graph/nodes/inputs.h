#ifndef PCG_NODES_INPUTS_H
#define PCG_NODES_INPUTS_H

#include <span>
#include "../node_types.h"

namespace PCG {

void RegisterInputNodes(
    std::span<NodeType, static_cast<size_t>(NodeTypeID::COUNT)> types) {
  NodeType& t = types[static_cast<size_t>(NodeTypeID::InputX)];
  t.name = "Input X";
  t.category = Category::INPUT;
  t.outputs = {{"x"}};
}

}  // namespace PCG

#endif  // PCG_NODES_INPUTS_H