#include "model.h"

#include "engine/core/logger.h"
#include "engine/formats/gltf_translator.h"

std::shared_ptr<Model> Model::Load(const std::string& file) {
  auto out = std::make_shared<Model>();
  auto md = Formats::TranslateGLTF(file);

  if (md.empty()) {
    Logger::getInstance().Log(LogLevel::Error,
                              "[Model::Load] failed - no meshes");
    return nullptr;
  }

  out->meshes_.reserve(md.size());
  for (auto& mesh_data : md)
    out->meshes_.emplace_back(std::make_unique<Mesh>(mesh_data));

  // debug
  Logger::getInstance().Log(LogLevel::Debug,
                            "[Model] Model loaded, mesh count: " +
                                std::to_string(out->NLoadedMeshes()));

  return out;
}

// Mesh object number inside model
uint32_t Model::NLoadedMeshes() const {
  return static_cast<uint32_t>(meshes_.size());
}

// Return pointer to mesh by index
const Mesh* Model::QueryMesh(uint32_t index) const {
  return index < meshes_.size() ? meshes_[index].get() : nullptr;
}

void Model::Draw(bgfx::ViewId view, bgfx::ProgramHandle program) const {
  for (const auto& mesh_data : meshes_) {
    mesh_data->Bind();
    bgfx::setState(BGFX_STATE_DEFAULT);  // opaque
    bgfx::submit(view, program);
  }
}
