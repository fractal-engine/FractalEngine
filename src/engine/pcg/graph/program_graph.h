#ifndef PROGRAM_GRAPH_H
#define PROGRAM_GRAPH_H

#include <cassert>
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace PCG {

// Core data structure
class ProgramGraph {
public:
  static const uint32_t NULL_ID = 0;
  static const uint32_t NULL_INDEX = -1;

  struct PortLocation {
    uint32_t node_id;
    uint32_t port_index;

    bool operator==(const PortLocation& other) const {
      return node_id == other.node_id && port_index == other.port_index;
    }
  };

  struct Connection {
    PortLocation src;
    PortLocation dst;
  };

  struct Port {
    std::vector<PortLocation> connections;
    std::string dynamic_name;  // For expression nodes with variable inputs
    uint32_t autoconnect_hint = 0;

    bool IsDynamic() const { return !dynamic_name.empty(); }
  };

  struct Node {
    uint32_t id;
    uint32_t type_id;
    std::string name;  // User-defined name
    glm::vec2 gui_position;
    glm::vec2 gui_size;  // For resizable nodes
    std::vector<Port> inputs;
    std::vector<Port> outputs;
    std::vector<std::variant<float, int, bool, std::string>> params;
    std::vector<float> default_inputs;
    bool autoconnect_default_inputs = false;

    uint32_t FindInputConnection(PortLocation src,
                                 uint32_t input_port_index) const;
    uint32_t FindOutputConnection(uint32_t output_port_index,
                                  PortLocation dst) const;
  };

  ~ProgramGraph();

  Node* CreateNode(uint32_t type_id, uint32_t id = NULL_ID);
  Node& GetNode(uint32_t id) const;
  Node* TryGetNode(uint32_t id) const;

  void RemoveNode(uint32_t id);
  void Clear();
  void CopyFrom(const ProgramGraph& other);

  bool IsConnected(PortLocation src, PortLocation dst) const;
  bool CanConnect(PortLocation src, PortLocation dst) const;
  bool IsValidConnection(PortLocation src, PortLocation dst) const;
  void Connect(PortLocation src, PortLocation dst);
  bool Disconnect(PortLocation src, PortLocation dst);

  bool HasPath(uint32_t src_node_id,
               uint32_t dst_node_id) const;  // Cycle detection
  void GetConnections(std::vector<Connection>& out_connections) const;
  void GetNodeIds(std::vector<uint32_t>& out_node_ids) const;

  uint32_t GenerateNodeId();
  unsigned int GetNodesCount() const;

  template <typename F>
  void ForEachNode(F f) const {
    for (auto& [id, node] : _nodes) {
      f(*node);
    }
  }

private:
  std::unordered_map<uint32_t, Node*> _nodes;
  uint32_t _next_node_id = 1;
};

}  // namespace PCG

#endif  // PROGRAM_GRAPH_H