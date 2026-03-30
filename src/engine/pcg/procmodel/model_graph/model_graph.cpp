#include "model_graph.h"

namespace ProcModel {

bool ModelGraph::UploadMeshes() {
  gpu_meshes.reserve(meshes.size());
  for (const auto& md : meshes) {
    gpu_meshes.emplace_back(std::make_unique<Mesh>(md));
  }
  return !gpu_meshes.empty();
}

const Mesh* ModelGraph::GetMesh(int index) const {
  if (index < 0 || index >= static_cast<int>(gpu_meshes.size()))
    return nullptr;
  return gpu_meshes[index].get();
}

}  // namespace ProcModel
