#include "pcg_engine.h"

#include "engine/content/loaders/mesh_loader.h"
#include "engine/context/engine_context.h"
#include "engine/ecs/components/mesh_renderer_component.h"
#include "engine/ecs/components/volume_component.h"
#include "engine/ecs/world.h"

#include "engine/pcg/procmodel/descriptor/descriptor_parser.h"
#include "engine/pcg/procmodel/generator/descriptor_resolver.h"
#include "engine/pcg/procmodel/generator/model_generator.h"
#include "engine/pcg/procmodel/instantiator/model_instantiator.h"
#include "engine/pcg/procmodel/model_graph/model_graph_builder.h"
#include "engine/pcg/procmodel/procmodel_resource.h"

#include "generator_resource.h"

void PCGEngine::ProcessQueued() {
  auto& ecs = ECS::Main();  // should not be called directly (tight coupling)
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
        // For now, we just mark it as processed

        volume.dirty = false;
      });

  // Clear processed requests
  while (!pending_requests_.empty()) {
    pending_requests_.pop();
  }
}

ResourceID PCGEngine::LoadArchetype(const std::string& descriptor_path) {
  auto cache_it = procmodel_cache_.find(descriptor_path);
  if (cache_it != procmodel_cache_.end())
    return cache_it->second;

  auto& resource_mgr = EngineContext::resourceManager();

  auto [id, resource] =
      resource_mgr.Create<ProcModel::ProcModelResource>(descriptor_path);

  if (!ProcModel::DescriptorParser::LoadFromFile(descriptor_path,
                                                 resource->GetDescriptor())) {
    Logger::getInstance().Log(
        LogLevel::Error,
        "[PCGEngine] Failed to parse descriptor: " + descriptor_path);
    resource_mgr.Release(id);
    return 0;
  }

  const auto& descriptor = resource->GetDescriptor();
  Content::SceneData scene = Content::MeshLoader::LoadScene(descriptor.path);
  if (scene.meshes.empty()) {
    Logger::getInstance().Log(
        LogLevel::Error,
        "[PCGEngine] Failed to load scene: " + descriptor.path);
    resource_mgr.Release(id);
    return 0;
  }

  resource->GetGraph() =
      ProcModel::ModelGraphBuilder::Build(scene, descriptor.path);

  if (!resource->GetGraph().UploadMeshes()) {
    Logger::getInstance().Log(LogLevel::Error,
                              "[PCGEngine] Failed to upload meshes");
    resource_mgr.Release(id);
    return 0;
  }

  auto resolve_result = ProcModel::DescriptorResolver::Resolve(
      resource->GetGraph(), resource->GetDescriptor());
  if (!resolve_result.success) {
    Logger::getInstance().Log(LogLevel::Error,
                              "[PCGEngine] Descriptor resolution failed");
    resource_mgr.Release(id);
    return 0;
  }

  resource->SetResolved(true);
  procmodel_cache_[descriptor_path] = id;
  return id;
}

// TODO: move load/build/resolve steps into a background ResourcePipe task,
// and the GPU upload into a UseRenderThread task. This will be needed
// so the pipeline doesn't block the frame
ProcModel::ModelInstantiator::InstantiateResult PCGEngine::RequestInstance(
    const std::string& descriptor_path, uint64_t seed, Entity parent) {

  ResourceID resource_id = LoadArchetype(descriptor_path);
  if (resource_id == 0)
    return {};

  auto& resource_mgr = EngineContext::resourceManager();
  auto resource =
      resource_mgr.GetResourceAs<ProcModel::ProcModelResource>(resource_id);
  if (!resource || !resource->IsResolved()) {
    Logger::getInstance().Log(LogLevel::Error,
                              "[PCGEngine] Resource not ready");
    return {};
  }

  // Generate instance
  auto resolved = ProcModel::ModelGenerator::Generate(
      resource->GetGraph(), resource->GetDescriptor(), seed, 10,
      &ValidationLog());
  if (!resolved) {
    Logger::getInstance().Log(
        LogLevel::Warning,
        "[PCGEngine] Generation failed after max retries for seed: " +
            std::to_string(seed));
    return {};
  }

  // Instantiate into ECS
  return ProcModel::ModelInstantiator::Instantiate(
      *resolved, resource->GetGraph(), parent);
}
