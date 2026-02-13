#include "model.h"

#include "engine/core/logger.h"
#include "engine/formats/gltf_translator.h"

bool Model::LoadData() {
  auto md = Formats::TranslateGLTF(source_path_);

  if (md.empty()) {
    Logger::getInstance().Log(
        LogLevel::Error,
        "[Model::LoadData] failed - no meshes in " + source_path_);
    return false;
  }

  // Compute metrics while CPU data is available
  metrics_ = ComputeMetrics(md);

  // Store for upload phase
  mesh_data_ = std::move(md);

  Logger::getInstance().Log(
      LogLevel::Debug, "[Model] Data loaded: " + source_path_ +
                           ", meshes: " + std::to_string(mesh_data_.size()));
  return true;
}

bool Model::UploadBuffers() {
  if (mesh_data_.empty()) {
    Logger::getInstance().Log(LogLevel::Error,
                              "[Model::UploadBuffers] no data to upload");
    return false;
  }

  meshes_.reserve(mesh_data_.size());
  for (auto& md : mesh_data_)
    meshes_.emplace_back(std::make_unique<Mesh>(md));

  // Free intermediate CPU data
  mesh_data_.clear();

  Logger::getInstance().Log(LogLevel::Debug,
                            "[Model] Buffers uploaded, mesh count: " +
                                std::to_string(NLoadedMeshes()));
  return true;
}

void Model::Destroy() {
  meshes_.clear();
  mesh_data_.clear();
  metrics_ = Metrics{};
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

Model::Metrics Model::ComputeMetrics(
    const std::vector<Resources3D::MeshData>& mesh_data) {
  Metrics m{};
  m.n_meshes = static_cast<uint32_t>(mesh_data.size());

  glm::vec3 centroid_acc(0.0f);

  for (const auto& md : mesh_data) {
    const size_t v_count = md.positions_.size() / 3;
    m.n_vertices += static_cast<uint32_t>(v_count);
    m.n_faces += static_cast<uint32_t>(md.indices_.size()) / 3;

    for (size_t i = 0; i < v_count; i++) {
      glm::vec3 pos(md.positions_[i * 3 + 0], md.positions_[i * 3 + 1],
                    md.positions_[i * 3 + 2]);

      m.min_point = glm::min(m.min_point, pos);
      m.max_point = glm::max(m.max_point, pos);
      m.furthest = glm::max(m.furthest, glm::length(pos));
      centroid_acc += pos;
    }
  }

  if (m.n_vertices > 0) {
    m.centroid = centroid_acc / static_cast<float>(m.n_vertices);
  }

  m.origin = (m.min_point + m.max_point) * 0.5f;
  return m;
}
