#ifndef GRAPH_COMPILER_H
#define GRAPH_COMPILER_H

#include <vector>

#include "node_types.h"
#include "program_graph.h"

namespace PCG {

struct CompiledGraph {
  // Nodes in topological order
  std::vector<uint32_t> execution_order;

  // Buffer indices for each node's outputs
  std::vector<std::vector<uint32_t>> output_buffers;

  // Total buffer count
  uint32_t buffer_count = 0;
};

class GraphCompiler {
public:
  static bool Compile(const ProgramGraph& graph, uint32_t output_node_id,
                      CompiledGraph& out_compiled);

private:
  static bool TopologicalSort(const ProgramGraph& graph,
                              uint32_t output_node_id,
                              std::vector<uint32_t>& out_order);
};

}  // namespace PCG

#endif  // GRAPH_COMPILER_H