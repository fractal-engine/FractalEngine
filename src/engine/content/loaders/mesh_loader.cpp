#include "mesh_loader.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include "engine/core/logger.h"

namespace Content {

namespace {

Geometry::MeshData ConvertMesh(const aiMesh* ai_mesh) {
  Geometry::MeshData out;
  out.Reserve(ai_mesh->mNumVertices, ai_mesh->mNumFaces * 3);

  // Positions
  for (unsigned int i = 0; i < ai_mesh->mNumVertices; ++i) {
    const auto& v = ai_mesh->mVertices[i];
    out.positions.push_back(v.x);
    out.positions.push_back(v.y);
    out.positions.push_back(v.z);
  }

  // Normals
  if (ai_mesh->HasNormals()) {
    for (unsigned int i = 0; i < ai_mesh->mNumVertices; ++i) {
      const auto& n = ai_mesh->mNormals[i];
      out.normals.push_back(n.x);
      out.normals.push_back(n.y);
      out.normals.push_back(n.z);
    }
  }

  // UVs
  if (ai_mesh->HasTextureCoords(0)) {
    for (unsigned int i = 0; i < ai_mesh->mNumVertices; ++i) {
      const auto& uv = ai_mesh->mTextureCoords[0][i];
      out.tex_coords.push_back(uv.x);
      out.tex_coords.push_back(uv.y);
    }
  }

  // Vertex colors
  if (ai_mesh->HasVertexColors(0)) {
    for (unsigned int i = 0; i < ai_mesh->mNumVertices; ++i) {
      const auto& c = ai_mesh->mColors[0][i];
      out.colors.push_back(c.r);
      out.colors.push_back(c.g);
      out.colors.push_back(c.b);
      out.colors.push_back(c.a);
    }
  }

  // Indices
  for (unsigned int f = 0; f < ai_mesh->mNumFaces; ++f) {
    const aiFace& face = ai_mesh->mFaces[f];
    for (unsigned int j = 0; j < face.mNumIndices; ++j) {
      out.indices.push_back(face.mIndices[j]);
    }
  }

  return out;
}

void ProcessNode(const aiNode* node, const aiScene* scene,
                 std::vector<Geometry::MeshData>& out_meshes) {
  for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
    const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    out_meshes.push_back(ConvertMesh(mesh));
  }

  for (unsigned int i = 0; i < node->mNumChildren; ++i) {
    ProcessNode(node->mChildren[i], scene, out_meshes);
  }
}

}  // namespace

std::vector<Geometry::MeshData> MeshLoader::Load(const std::string& path) {
  Assimp::Importer importer;

  const aiScene* scene = importer.ReadFile(
      path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                aiProcess_JoinIdenticalVertices |
                aiProcess_ImproveCacheLocality);

  if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) ||
      !scene->mRootNode) {
    Logger::getInstance().Log(LogLevel::Error,
                              "[MeshLoader] Failed to load: " + path + " - " +
                                  importer.GetErrorString());
    return {};
  }

  std::vector<Geometry::MeshData> meshes;
  ProcessNode(scene->mRootNode, scene, meshes);

  Logger::getInstance().Log(LogLevel::Debug, "[MeshLoader] Loaded " +
                                                 std::to_string(meshes.size()) +
                                                 " meshes from " + path);

  return meshes;
}

bool MeshLoader::IsSupported(const std::string& path) {
  Assimp::Importer importer;
  return importer.IsExtensionSupported(path.substr(path.rfind('.')));
}

}  // namespace Content