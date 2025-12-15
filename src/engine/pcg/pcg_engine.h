#ifndef PCG_ENGINE_H
#define PCG_ENGINE_H

#include <functional>
#include <queue>
#include <unordered_set>

#include "engine/pcg/graph/program_graph.h"

struct GenerationRequest {
  uint32_t volume_id;    // Entity ID (VolumeComponent lookup)
  uint8_t priority = 0;  // Higher = sooner
};

class PCGEngine {
public:
  // ─────────────────────────────────────────────────────────────
  // GENERATION QUEUE
  // ─────────────────────────────────────────────────────────────
  void RequestGeneration(uint32_t volume_entity_id, uint8_t priority = 0) {
    pending_requests_.push({volume_entity_id, priority});
  }

  void CancelGeneration(uint32_t volume_entity_id) {
    cancelled_.insert(volume_entity_id);
  }

  // ? Called from EngineContext::NextFrame()
  void ProcessQueued();

  // ─────────────────────────────────────────────────────────────
  // GRAPH EDITOR STATE
  // ─────────────────────────────────────────────────────────────
  // Currently edited graph (set to null if no graph is edited)
  PCG::ProgramGraph* active_graph = nullptr;

  // Graph editor visibility
  bool show_graph_editor = false;

  void SetActiveGraph(PCG::ProgramGraph* graph) {
    active_graph = graph;
    show_graph_editor = (graph != nullptr);
  }

  void CloseGraphEditor() {
    active_graph = nullptr;
    show_graph_editor = false;
  }

private:
  std::queue<GenerationRequest> pending_requests_;
  std::unordered_set<uint32_t> cancelled_;
};

#endif  // PCG_ENGINE_H