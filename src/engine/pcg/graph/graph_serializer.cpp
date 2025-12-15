#include "graph_serializer.h"
#include "node_types.h"

#include <fstream>
#include <iostream>

namespace PCG {

nlohmann::json GraphSerializer::ToJson(const ProgramGraph& graph) {
  nlohmann::json j;
  j["version"] = 1;

  // Serialize nodes
  nlohmann::json nodes_json = nlohmann::json::object();

  graph.ForEachNode([&](const ProgramGraph::Node& node) {
    const NodeType& type = NodeTypeDB::Instance().Get(node.type_id);

    nlohmann::json node_json;
    node_json["type"] = type.name;
    node_json["name"] = node.name;
    node_json["gui_position"] = {node.gui_position.x, node.gui_position.y};

    if (node.gui_size.x > 0 && node.gui_size.y > 0) {
      node_json["gui_size"] = {node.gui_size.x, node.gui_size.y};
    }

    // Serialize parameters
    nlohmann::json params_json = nlohmann::json::object();
    for (size_t i = 0; i < node.params.size() && i < type.params.size(); ++i) {
      const auto& param_def = type.params[i];
      const auto& param_val = node.params[i];

      std::visit([&](auto&& val) { params_json[param_def.name] = val; },
                 param_val);
    }
    if (!params_json.empty()) {
      node_json["params"] = params_json;
    }

    // Serialize default inputs
    nlohmann::json defaults_json = nlohmann::json::object();
    for (size_t i = 0; i < node.default_inputs.size() && i < type.inputs.size();
         ++i) {
      if (node.default_inputs[i] != type.inputs[i].default_value) {
        defaults_json[type.inputs[i].name] = node.default_inputs[i];
      }
    }
    if (!defaults_json.empty()) {
      node_json["default_inputs"] = defaults_json;
    }

    nodes_json[std::to_string(node.id)] = node_json;
  });
  j["nodes"] = nodes_json;

  // Serialize connections as [src_node, src_port, dst_node, dst_port]
  std::vector<ProgramGraph::Connection> connections;
  graph.GetConnections(connections);

  nlohmann::json connections_json = nlohmann::json::array();
  for (const auto& conn : connections) {
    connections_json.push_back({conn.src.node_id, conn.src.port_index,
                                conn.dst.node_id, conn.dst.port_index});
  }
  j["connections"] = connections_json;

  return j;
}

bool GraphSerializer::FromJson(const nlohmann::json& j,
                               ProgramGraph& out_graph) {
  out_graph.Clear();

  int version = j.value("version", 1);

  // Deserialize nodes
  if (j.contains("nodes")) {
    for (auto& [id_str, node_json] : j["nodes"].items()) {
      uint32_t id = std::stoul(id_str);

      // Look up type by name
      std::string type_name = node_json["type"];
      NodeTypeID type_id;
      if (!NodeTypeDB::Instance().TryGetId(type_name, type_id)) {

        // ! CHANGE THIS:
        std::cerr << "Unknown node type: " << type_name << std::endl;
        continue;
      }

      const NodeType& type = NodeTypeDB::Instance().Get(type_id);
      ProgramGraph::Node* node =
          out_graph.CreateNode(static_cast<uint32_t>(type_id), id);

      // Set up ports based on type
      node->inputs.resize(type.inputs.size());
      node->outputs.resize(type.outputs.size());
      node->default_inputs.resize(type.inputs.size());
      node->params.resize(type.params.size());

      // Initialize defaults
      for (size_t i = 0; i < type.inputs.size(); ++i) {
        node->default_inputs[i] = type.inputs[i].default_value;
      }
      for (size_t i = 0; i < type.params.size(); ++i) {
        node->params[i] = type.params[i].default_value;
      }

      // Load user-defined name
      if (node_json.contains("name")) {
        node->name = node_json["name"];
      }

      // Load position
      if (node_json.contains("gui_position")) {
        auto& pos = node_json["gui_position"];
        node->gui_position = glm::vec2(pos[0], pos[1]);
      }

      // Load size (for resizable nodes)
      if (node_json.contains("gui_size")) {
        auto& size = node_json["gui_size"];
        node->gui_size = glm::vec2(size[0], size[1]);
      }

      // Load parameters
      if (node_json.contains("params")) {
        for (size_t i = 0; i < type.params.size(); ++i) {
          const auto& param_def = type.params[i];
          if (node_json["params"].contains(param_def.name)) {
            auto& val = node_json["params"][param_def.name];
            switch (param_def.type) {
              case NodeType::Param::Type::Float:
                node->params[i] = val.get<float>();
                break;
              case NodeType::Param::Type::Int:
                node->params[i] = val.get<int>();
                break;
              case NodeType::Param::Type::Bool:
                node->params[i] = val.get<bool>();
                break;
              case NodeType::Param::Type::Enum:
                node->params[i] = val.get<std::string>();
                break;
            }
          }
        }
      }

      // Load default inputs
      if (node_json.contains("default_inputs")) {
        for (size_t i = 0; i < type.inputs.size(); ++i) {
          if (node_json["default_inputs"].contains(type.inputs[i].name)) {
            node->default_inputs[i] =
                node_json["default_inputs"][type.inputs[i].name].get<float>();
          }
        }
      }
    }
  }

  // Deserialize connections
  if (j.contains("connections")) {
    for (const auto& conn_json : j["connections"]) {
      ProgramGraph::PortLocation src{conn_json[0], conn_json[1]};
      ProgramGraph::PortLocation dst{conn_json[2], conn_json[3]};

      if (out_graph.CanConnect(src, dst)) {
        out_graph.Connect(src, dst);
      }
    }
  }

  return true;
}

bool GraphSerializer::SaveToFile(const ProgramGraph& graph,
                                 const std::string& path) {
  try {
    nlohmann::json j = ToJson(graph);
    std::ofstream file(path);
    if (!file.is_open()) {
      return false;
    }
    file << j.dump(2);  // Pretty print with 2-space indent
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Failed to save graph: " << e.what() << std::endl;
    return false;
  }
}

bool GraphSerializer::LoadFromFile(const std::string& path,
                                   ProgramGraph& out_graph) {
  try {
    std::ifstream file(path);
    if (!file.is_open()) {
      return false;
    }
    nlohmann::json j = nlohmann::json::parse(file);
    return FromJson(j, out_graph);
  } catch (const std::exception& e) {
    std::cerr << "Failed to load graph: " << e.what() << std::endl;
    return false;
  }
}

}  // namespace PCG