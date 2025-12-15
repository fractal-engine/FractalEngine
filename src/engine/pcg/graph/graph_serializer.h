#ifndef GRAPH_SERIALIZER_H
#define GRAPH_SERIALIZER_H

#include <nlohmann/json.hpp>
#include <string>

#include "program_graph.h"

namespace PCG {

class GraphSerializer {
public:
  static nlohmann::json ToJson(const ProgramGraph& graph);
  static bool FromJson(const nlohmann::json& j, ProgramGraph& out_graph);

  static bool SaveToFile(const ProgramGraph& graph, const std::string& path);
  static bool LoadFromFile(const std::string& path, ProgramGraph& out_graph);
};

}  // namespace PCG

#endif  // GRAPH_SERIALIZER_H