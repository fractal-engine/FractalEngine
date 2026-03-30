#ifndef MODEL_GRAPH_H
#define MODEL_GRAPH_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/core/types/geometry_data.h"
#include "engine/renderer/model/mesh.h"

#include "model_graph_node.h"

namespace ProcModel {

struct ModelGraph {
  std::string source_path;
  ModelGraphNode root;
  std::vector<Geometry::MeshData> meshes;

  // Flat lookup for DescriptorResolver
  std::unordered_map<std::string, ModelGraphNode*> node_lookup;

  // GPU-uploaded meshes
  std::vector<std::unique_ptr<Mesh>> gpu_meshes;

  bool UploadMeshes();
  const Mesh* GetMesh(int index) const;
};

}  // namespace ProcModel

#endif  // MODEL_GRAPH_H
