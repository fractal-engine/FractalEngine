#include "pcg_engine.h"

#include "engine/context/engine_context.h"
#include "engine/ecs/components/mesh_renderer_component.h"
#include "engine/ecs/components/volume_component.h"
#include "engine/ecs/world.h"
#include "generator_resource.h"

void PCGEngine::ProcessQueued() {
  auto& ecs = ECS::Main();
  auto& resource_mgr = EngineContext::resourceManager();

  // Process all dirty volumes
  ecs.View<VolumeComponent, MeshRendererComponent>().each(
      [&](Entity entity, VolumeComponent& volume,
          MeshRendererComponent& mesh_renderer) {
        if (!volume.dirty)
          return;

        // Check if cancelled
        if (cancelled_.count(static_cast<uint32_t>(entity))) {
          cancelled_.erase(static_cast<uint32_t>(entity));
          return;
        }

        // Get generator resource
        auto gen_ref = resource_mgr.GetResourceAs<PCG::GeneratorResource>(
            volume.generator_id);
        if (!gen_ref || !gen_ref->Get())
          return;

        auto* generator = gen_ref->Get();

        // TODO: Generate mesh using generator->Eval()
        // For now, just mark as processed

        volume.dirty = false;
      });

  // Clear processed requests
  while (!pending_requests_.empty()) {
    pending_requests_.pop();
  }
}