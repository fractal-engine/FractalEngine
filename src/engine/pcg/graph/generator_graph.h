#ifndef GENERATOR_GRAPH_H
#define GENERATOR_GRAPH_H

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

#include <glm/glm.hpp>

#include "engine/pcg/core/sample.h"
#include "graph_runtime.h"
#include "program_graph.h"

/*******************************************************************************
 * GENERATOR GRAPH
 * TODO:
 * - Output index extraction during compile (needs iteration over output nodes)
 * - Batch processing in GenerateSeries (currently loops single queries)
 * - Implement range analysis for optimization
 * - Create a Cache struct
 ******************************************************************************/
namespace PCG {

class NodeTypeDB;

class GeneratorGraph {
public:
  GeneratorGraph();
  ~GeneratorGraph();

  // Graph editing
  void Clear();
  ProgramGraph& GetGraph() { return graph_; }
  const ProgramGraph& GetGraph() const { return graph_; }

  // Compilation
  CompilationResult Compile();
  bool IsCompiled() const;

  // Generation - single point query
  Sample GenerateSingle(glm::vec2 position) const;

  // Generation - batch query (better for multiple points)
  void GenerateSeries(const std::vector<glm::vec2>& positions,
                      std::vector<Sample>& out_samples) const;

  // Generation - grid query for heightmap generation
  void GenerateGrid(glm::vec2 origin, glm::vec2 size, glm::ivec2 resolution,
                    std::vector<Sample>& out_samples) const;

  // Clone for thread-local usage
  std::unique_ptr<GeneratorGraph> Clone() const;

private:
  // Cache for runtime state
  struct Cache {
    GraphRuntime::State state;
  };
  static Cache& GetTLSCache();

  // Wrapper around GraphRuntime for metada
  struct Runtime {
    GraphRuntime runtime;

    // Output buffer indices
    int height_output_index = -1;
    int slope_output_index = -1;
  };

  ProgramGraph graph_;
  std::shared_ptr<Runtime> runtime_;
  mutable std::shared_mutex runtime_mutex_;
};
}  // namespace PCG

#endif  // GENERATOR_GRAPH_H