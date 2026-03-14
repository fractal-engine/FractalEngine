#include "program_graph.h"

#include <algorithm>

namespace PCG {

ProgramGraph::~ProgramGraph() {
  Clear();
}

void ProgramGraph::Clear() {
  for (auto& [id, node] : _nodes)
    delete node;
  _nodes.clear();
  _next_node_id = 1;
}

ProgramGraph::Node* ProgramGraph::CreateNode(uint32_t type_id, uint32_t id) {
  if (id == NULL_ID) {
    id = GenerateNodeId();
  } else {
    assert(_nodes.find(id) == _nodes.end());
    if (_next_node_id <= id) {
      _next_node_id = id + 1;
    }
  }
  Node* node = new Node();
  node->id = id;
  node->type_id = type_id;
  _nodes[node->id] = node;
  return node;
}

void ProgramGraph::RemoveNode(uint32_t id) {
  Node* node = TryGetNode(id);
  if (!node)
    return;

  // Disconnect all inputs
  for (size_t i = 0; i < node->inputs.size(); ++i) {
    for (const PortLocation& src : node->inputs[i].connections) {
      Node& src_node = GetNode(src.node_id);
      auto& conns = src_node.outputs[src.port_index].connections;
      conns.erase(std::remove_if(
                      conns.begin(), conns.end(),
                      [&](const PortLocation& p) { return p.node_id == id; }),
                  conns.end());
    }
  }

  // Disconnect all outputs
  for (size_t i = 0; i < node->outputs.size(); ++i) {
    for (const PortLocation& dst : node->outputs[i].connections) {
      Node& dst_node = GetNode(dst.node_id);
      auto& conns = dst_node.inputs[dst.port_index].connections;
      conns.erase(std::remove_if(
                      conns.begin(), conns.end(),
                      [&](const PortLocation& p) { return p.node_id == id; }),
                  conns.end());
    }
  }

  delete node;
  _nodes.erase(id);
}

ProgramGraph::Node& ProgramGraph::GetNode(uint32_t id) const {
  auto it = _nodes.find(id);
  assert(it != _nodes.end());
  return *it->second;
}

ProgramGraph::Node* ProgramGraph::TryGetNode(uint32_t id) const {
  auto it = _nodes.find(id);
  return (it != _nodes.end()) ? it->second : nullptr;
}

void ProgramGraph::Connect(PortLocation src, PortLocation dst) {
  assert(!IsConnected(src, dst));
  assert(src.node_id != dst.node_id);          // No self-connections
  assert(!HasPath(dst.node_id, src.node_id));  // No cycles

  Node& src_node = GetNode(src.node_id);
  Node& dst_node = GetNode(dst.node_id);

  // Single input connection per port
  assert(dst_node.inputs[dst.port_index].connections.empty());

  src_node.outputs[src.port_index].connections.push_back(dst);
  dst_node.inputs[dst.port_index].connections.push_back(src);
}

unsigned int ProgramGraph::GetNodesCount() const {
  return static_cast<unsigned int>(_nodes.size());
}

uint32_t ProgramGraph::GenerateNodeId() {
  return _next_node_id++;
}

bool ProgramGraph::IsConnected(PortLocation src, PortLocation dst) const {
  const Node* src_node = TryGetNode(src.node_id);
  if (!src_node)
    return false;

  const auto& conns = src_node->outputs[src.port_index].connections;
  return std::find(conns.begin(), conns.end(), dst) != conns.end();
}

bool ProgramGraph::CanConnect(PortLocation src, PortLocation dst) const {
  if (src.node_id == dst.node_id)
    return false;
  if (IsConnected(src, dst))
    return false;
  if (HasPath(dst.node_id, src.node_id))
    return false;

  const Node* dst_node = TryGetNode(dst.node_id);
  if (!dst_node)
    return false;

  // Input port must be empty (single connection per input)
  return dst_node->inputs[dst.port_index].connections.empty();
}

bool ProgramGraph::Disconnect(PortLocation src, PortLocation dst) {
  Node* src_node = TryGetNode(src.node_id);
  Node* dst_node = TryGetNode(dst.node_id);
  if (!src_node || !dst_node)
    return false;

  auto& out_conns = src_node->outputs[src.port_index].connections;
  auto it = std::remove_if(out_conns.begin(), out_conns.end(),
                           [&](const PortLocation& p) { return p == dst; });
  if (it == out_conns.end())
    return false;
  out_conns.erase(it, out_conns.end());

  auto& in_conns = dst_node->inputs[dst.port_index].connections;
  in_conns.erase(
      std::remove_if(in_conns.begin(), in_conns.end(),
                     [&](const PortLocation& p) { return p == src; }),
      in_conns.end());

  return true;
}

bool ProgramGraph::HasPath(uint32_t src_node_id, uint32_t dst_node_id) const {
  // BFS/DFS to detect if connecting creates a cycle
  std::vector<uint32_t> to_process;
  std::unordered_set<uint32_t> visited;

  to_process.push_back(src_node_id);

  while (!to_process.empty()) {
    const Node& node = GetNode(to_process.back());
    to_process.pop_back();
    visited.insert(node.id);

    for (const auto& output : node.outputs) {
      for (const auto& conn : output.connections) {
        if (conn.node_id == dst_node_id) {
          return true;
        }
        if (visited.find(conn.node_id) == visited.end()) {
          to_process.push_back(conn.node_id);
        }
      }
    }
  }
  return false;
}

void ProgramGraph::GetNodeIds(std::vector<uint32_t>& out) const {
  out.clear();
  out.reserve(_nodes.size());
  for (const auto& [id, _] : _nodes)
    out.push_back(id);
}

void ProgramGraph::CopyFrom(const ProgramGraph& other) {
  Clear();

  // Copy nodes
  for (const auto& [id, src_node] : other._nodes) {
    Node* dst_node = new Node();
    dst_node->id = src_node->id;
    dst_node->type_id = src_node->type_id;
    dst_node->name = src_node->name;
    dst_node->gui_position = src_node->gui_position;
    dst_node->gui_size = src_node->gui_size;
    dst_node->params = src_node->params;
    dst_node->default_inputs = src_node->default_inputs;
    dst_node->autoconnect_default_inputs = src_node->autoconnect_default_inputs;

    // Copy port structure without connections
    dst_node->inputs.resize(src_node->inputs.size());
    for (size_t i = 0; i < src_node->inputs.size(); ++i) {
      dst_node->inputs[i].dynamic_name = src_node->inputs[i].dynamic_name;
      dst_node->inputs[i].autoconnect_hint =
          src_node->inputs[i].autoconnect_hint;
    }

    dst_node->outputs.resize(src_node->outputs.size());
    for (size_t i = 0; i < src_node->outputs.size(); ++i) {
      dst_node->outputs[i].dynamic_name = src_node->outputs[i].dynamic_name;
      dst_node->outputs[i].autoconnect_hint =
          src_node->outputs[i].autoconnect_hint;
    }

    _nodes[dst_node->id] = dst_node;
  }

  // Rebuild connections
  for (const auto& [id, src_node] : other._nodes) {
    for (size_t output_index = 0; output_index < src_node->outputs.size();
         ++output_index) {
      const Port& src_port = src_node->outputs[output_index];
      for (const PortLocation& dst_loc : src_port.connections) {
        Connect(PortLocation{src_node->id, static_cast<uint32_t>(output_index)},
                dst_loc);
      }
    }
  }

  _next_node_id = other._next_node_id;
}

void ProgramGraph::GetConnections(std::vector<Connection>& connections) const {
  for (const auto& [id, node] : _nodes) {
    for (size_t i = 0; i < node->outputs.size(); ++i) {
      const Port& port = node->outputs[i];
      for (const auto& dst : port.connections) {
        connections.push_back(
            Connection{PortLocation{node->id, (uint32_t)i}, dst});
      }
    }
  }
}

}  // namespace PCG