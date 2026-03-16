#include "mesh_loader.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include "engine/core/logger.h"

namespace Content {

namespace {

Geometry::MeshData ConvertMesh(const aiMesh* ai_mesh) {
  Geometry::MeshData out;
  out.vertices.reserve(ai_mesh->mNumVertices);
  out.material_index = ai_mesh->mMaterialIndex;

  for (unsigned int i = 0; i < ai_mesh->mNumVertices; ++i) {
    Geometry::VertexData v;
    v.position = {ai_mesh->mVertices[i].x, ai_mesh->mVertices[i].y,
                  ai_mesh->mVertices[i].z};
    v.normal = ai_mesh->HasNormals()
                   ? glm::vec3(ai_mesh->mNormals[i].x, ai_mesh->mNormals[i].y,
                               ai_mesh->mNormals[i].z)
                   : glm::vec3(0.0f);
    v.uv = ai_mesh->HasTextureCoords(0)
               ? glm::vec2(ai_mesh->mTextureCoords[0][i].x,
                           ai_mesh->mTextureCoords[0][i].y)
               : glm::vec2(0.0f);
    v.tangent =
        ai_mesh->HasTangentsAndBitangents()
            ? glm::vec3(ai_mesh->mTangents[i].x, ai_mesh->mTangents[i].y,
                        ai_mesh->mTangents[i].z)
            : glm::vec3(0.0f);
    v.bitangent =
        ai_mesh->HasTangentsAndBitangents()
            ? glm::vec3(ai_mesh->mBitangents[i].x, ai_mesh->mBitangents[i].y,
                        ai_mesh->mBitangents[i].z)
            : glm::vec3(0.0f);
    out.vertices.push_back(v);
  }

  for (unsigned int f = 0; f < ai_mesh->mNumFaces; ++f) {
    const aiFace& face = ai_mesh->mFaces[f];
    for (unsigned int j = 0; j < face.mNumIndices; ++j)
      out.indices.push_back(face.mIndices[j]);
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
                aiProcess_ImproveCacheLocality | aiProcess_CalcTangentSpace);

  if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) ||
      !scene->mRootNode) {
    Logger::getInstance().Log(LogLevel::Error,
                              "[MeshLoader] Failed to load: " + path + " - " +
                                  importer.GetErrorString());

    // return interleaved vertices
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