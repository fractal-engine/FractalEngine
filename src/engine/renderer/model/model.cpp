#include "model.h"

#include "engine/content/cache/mesh_cache.h"
#include "engine/core/logger.h"

std::shared_ptr<Model> Model::Load(const std::string& file) {
  const auto& mesh_list = Content::MeshCache::Instance().Get(file);

  if (mesh_list.empty()) {
    Logger::getInstance().Log(LogLevel::Error,
                              "[Model::Load] No meshes in: " + file);
    return nullptr;
  }

  auto out = std::make_shared<Model>();
  out->meshes_.reserve(mesh_list.size());

  for (const auto& geom : mesh_list)
    out->meshes_.emplace_back(std::make_unique<Mesh>(geom));

  Logger::getInstance().Log(
      LogLevel::Debug,
      "[Model] Loaded " + std::to_string(out->NLoadedMeshes()) + " meshes");

  return out;
}

uint32_t Model::NLoadedMeshes() const {
  return static_cast<uint32_t>(meshes_.size());
}

const Mesh* Model::QueryMesh(uint32_t index) const {
  return index < meshes_.size() ? meshes_[index].get() : nullptr;
}

void Model::Draw(bgfx::ViewId view, bgfx::ProgramHandle program) const {
  for (const auto& mesh : meshes_) {
    mesh->Bind();
    bgfx::setState(BGFX_STATE_DEFAULT);  // opaque
    bgfx::submit(view, program);
  }
}