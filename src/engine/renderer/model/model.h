#ifndef MODEL_H
#define MODEL_H

#include <bgfx/bgfx.h>
#include <cfloat>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>

#include "engine/memory/resource.h"
#include "engine/renderer/model/mesh.h"
#include "engine/resources/3d/mesh_data.h"

class Model : public Resource {
public:
  struct Metrics {
    uint32_t n_meshes = 0;
    uint32_t n_faces = 0;
    uint32_t n_vertices = 0;

    glm::vec3 min_point = glm::vec3(FLT_MAX);
    glm::vec3 max_point = glm::vec3(-FLT_MAX);
    glm::vec3 origin = glm::vec3(0.0f);
    glm::vec3 centroid = glm::vec3(0.0f);
    float furthest = 0.0f;
  };

  const Metrics& GetMetrics() const { return metrics_; }

  // Resource pipeline for async loading
  ResourcePipe Create() {
    return std::move(
        Pipe() >> BIND_TASK(Model, LoadData) >>
        BIND_TASK_WITH_FLAGS(Model, UploadBuffers, TaskFlags::UseRenderThread));
  }

  // Set source file path
  void SetSource(const std::string& path) { source_path_ = path; }

  void Draw(bgfx::ViewId view, bgfx::ProgramHandle program) const;

  void Destroy() override;

  uint32_t NLoadedMeshes() const;

  const Mesh* QueryMesh(uint32_t index) const;

private:
  std::vector<std::unique_ptr<Mesh>> meshes_;

  static Metrics ComputeMetrics(
      const std::vector<Resources3D::MeshData>& mesh_data);

  Metrics metrics_;

  std::string source_path_;

  //  Parse file, compute metrics
  bool LoadData();

  // Create BGFX resources
  bool UploadBuffers();

  // Intermediate CPU data
  std::vector<Resources3D::MeshData> mesh_data_;
};

#endif  // MODEL_H
