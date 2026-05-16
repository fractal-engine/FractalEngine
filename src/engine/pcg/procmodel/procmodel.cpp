#include "procmodel.h"

#include "engine/core/logger.h"

#include "engine/content/io/json.h"
#include "engine/content/loaders/mesh_loader.h"
#include "engine/context/engine_context.h"

#include "engine/pcg/pipeline/linear_pipeline.h"
#include "engine/pcg/pipeline/pipeline_descriptor_parser.h"

#include "engine/pcg/procmodel/descriptor/model_descriptor_parser.h"
#include "engine/pcg/procmodel/generator/descriptor_resolver.h"
#include "engine/pcg/procmodel/generator/model_generator.h"
#include "engine/pcg/procmodel/model_graph/model_graph_builder.h"
#include "engine/pcg/procmodel/procmodel_resource.h"

namespace ProcModel {

Subsystem::Subsystem() = default;

void Subsystem::Init() {
  RegisterModelOperations(operation_registry_);

  Logger::getInstance().Log(
      LogLevel::Info,
      "[ProcModel::Subsystem] Init complete, registered " +
          std::to_string(
              operation_registry_.IsRegistered("vertex_deform") +
              operation_registry_.IsRegistered("duplicate_instances")) +
          " operations");
}

ResourceID Subsystem::LoadArchetype(const std::string& descriptor_path) {
  auto cache_it = procmodel_cache_.find(descriptor_path);
  if (cache_it != procmodel_cache_.end())
    return cache_it->second;

  auto& resource_mgr = EngineContext::resourceManager();

  auto [id, resource] =
      resource_mgr.Create<ProcModel::ProcModelResource>(descriptor_path);

  auto json_opt = Content::ReadJsonFile(descriptor_path);
  if (!json_opt) {
    resource_mgr.Release(id);
    return 0;
  }

  if (!ProcModel::ModelDescriptorParser::FromJson(*json_opt,
                                                  resource->GetDescriptor())) {
    Logger::getInstance().Log(
        LogLevel::Error, "[ProcModel::Subsystem] Failed to parse descriptor: " +
                             descriptor_path);
    resource_mgr.Release(id);
    return 0;
  }

  PCG::ParsePipelineDescriptor(*json_opt, resource->GetPipelineDescriptor());

  const auto& descriptor = resource->GetDescriptor();
  Content::SceneData scene = Content::MeshLoader::LoadScene(descriptor.path);
  if (scene.mesh_data.empty()) {
    Logger::getInstance().Log(
        LogLevel::Error,
        "[ProcModel::Subsystem] Failed to load scene: " + descriptor.path);
    resource_mgr.Release(id);
    return 0;
  }

  resource->GetGraph() =
      ProcModel::ModelGraphBuilder::Build(scene, descriptor.path);

  if (!resource->GetGraph().UploadMeshes()) {
    Logger::getInstance().Log(LogLevel::Error,
                              "[ProcModel::Subsystem] Failed to upload meshes");
    resource_mgr.Release(id);
    return 0;
  }

  auto resolve_result = ProcModel::DescriptorResolver::Resolve(
      resource->GetGraph(), resource->GetDescriptor());
  if (!resolve_result.success) {
    Logger::getInstance().Log(
        LogLevel::Error, "[ProcModel::Subsystem] Descriptor resolution failed");
    resource_mgr.Release(id);
    return 0;
  }

  {
    std::vector<std::string> errors;
    auto compiled = PCG::LinearPipeline::Compile(
        resource->GetPipelineDescriptor(), operation_registry_, errors);
    Logger::getInstance().Log(
        LogLevel::Info,
        "[ProcModel::Subsystem] Pipeline compiled with " +
            std::to_string(resource->GetPipelineDescriptor().entries.size()) +
            " entries");
    if (!compiled) {
      for (const auto& err : errors) {
        Logger::getInstance().Log(
            LogLevel::Error,
            "[ProcModel::Subsystem] Pipeline compilation: " + err);
      }
      resource_mgr.Release(id);
      return 0;
    }
    resource->GetPipeline() = std::move(*compiled);
  }

  resource->SetResolved(true);
  procmodel_cache_[descriptor_path] = id;
  return id;
}

// TODO: move load/build/resolve steps into a background ResourcePipe task,
// and the GPU upload into a UseRenderThread task. This will be needed
// so the pipeline doesn't block the frame
ProcModel::ModelInstantiator::InstantiateResult Subsystem::RequestInstance(
    const std::string& descriptor_path, uint64_t seed, Entity parent) {

  ResourceID resource_id = LoadArchetype(descriptor_path);
  if (resource_id == 0)
    return {};

  auto& resource_mgr = EngineContext::resourceManager();
  auto resource =
      resource_mgr.GetResourceAs<ProcModel::ProcModelResource>(resource_id);
  if (!resource || !resource->IsResolved()) {
    Logger::getInstance().Log(LogLevel::Error,
                              "[ProcModel::Subsystem] Resource not ready");
    return {};
  }

  // Generate instance
  auto resolved = ProcModel::ModelGenerator::Generate(
      resource->GetGraph(), resource->GetDescriptor(), resource->GetPipeline(),
      seed, 10, &ValidationLog());
  if (!resolved) {
    Logger::getInstance().Log(LogLevel::Warning,
                              "[ProcModel::Subsystem] Generation failed after "
                              "max retries for seed: " +
                                  std::to_string(seed));
    return {};
  }

  // Instantiate into ECS
  return ProcModel::ModelInstantiator::Instantiate(
      resolved->model, resource->GetGraph(), parent);
}

// Validation logger (one per session)
ValidationLogger& Subsystem::ValidationLog() {
  if (!validation_logger_)
    validation_logger_ = std::make_unique<ValidationLogger>();
  return *validation_logger_;
}

void Subsystem::Shutdown() {
  // registry is destroyed when the subsystem is destroyed
  // nothing to clean up
}

}  // namespace ProcModel