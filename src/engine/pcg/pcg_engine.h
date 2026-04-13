#ifndef PCG_ENGINE_H
#define PCG_ENGINE_H

#include <functional>
#include <queue>
#include <unordered_set>

#include "engine/pcg/graph/program_graph.h"
#include "engine/pcg/procmodel/instantiator/model_instantiator.h"
#include "engine/pcg/procmodel/procmodel_resource.h"

struct GenerationRequest {
  uint32_t volume_id;  // Entity ID (VolumeComponent lookup)
  uint8_t priority;    // Higher = sooner
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

  ProcModel::ModelInstantiator::InstantiateResult RequestInstance(
      const std::string& descriptor_path, uint64_t seed,
      Entity parent = entt::null);

  ResourceID LoadArchetype(const std::string& descriptor_path);

private:
  std::queue<GenerationRequest> pending_requests_;
  std::unordered_set<uint32_t> cancelled_;

  std::unordered_map<std::string, ResourceID> procmodel_cache_;
};

#endif  // PCG_ENGINE_H
