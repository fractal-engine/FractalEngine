#include "material_loader.h"

#include <assimp/DefaultIOSystem.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include "engine/core/logger.h"

namespace Content {

std::vector<MaterialData> MaterialLoader::ExtractMaterials(
    const aiScene* scene, const std::string& model_path) {
  std::vector<MaterialData> materials;
  materials.reserve(scene->mNumMaterials);

  std::string base_dir = model_path.substr(0, model_path.find_last_of("/\\"));

  // Query Assimp's shading model
  for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
    const aiMaterial* ai_mat = scene->mMaterials[i];
    MaterialData mat;

    aiString name;
    if (ai_mat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
      mat.name = name.C_Str();

    // Detect shading model to know which keys to query
    int shading_model = 0;
    ai_mat->Get(AI_MATKEY_SHADING_MODEL, shading_model);
    bool is_pbr = (shading_model == aiShadingMode_PBR_BRDF);

    // If shading model is unclear, check for PBR keys directly
    if (!is_pbr) {
      float metallic = 0.0f;
      is_pbr = (ai_mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS);
    }

    aiColor4D color;
    if (is_pbr) {
      if (ai_mat->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS)
        mat.base_color = glm::vec4(color.r, color.g, color.b, color.a);
    } else {
      if (ai_mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
        mat.base_color = glm::vec4(color.r, color.g, color.b, color.a);
    }

    // PBR factors
    if (is_pbr) {
      ai_mat->Get(AI_MATKEY_METALLIC_FACTOR, mat.metallic);
      ai_mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, mat.roughness);
    } else {
      // Map Phong specular to rough approximation
      float shininess = 0.0f;
      if (ai_mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
        mat.roughness = 1.0f - glm::clamp(shininess / 256.0f, 0.0f, 1.0f);
    }

    // Texture slots
    aiString tex;
    if (ai_mat->GetTexture(aiTextureType_DIFFUSE, 0, &tex) == AI_SUCCESS)
      mat.albedo_map = tex.C_Str();
    else if (ai_mat->GetTexture(aiTextureType_BASE_COLOR, 0, &tex) ==
             AI_SUCCESS)
      mat.albedo_map = tex.C_Str();

    if (ai_mat->GetTexture(aiTextureType_NORMALS, 0, &tex) == AI_SUCCESS)
      mat.normal_map = tex.C_Str();
    if (ai_mat->GetTexture(aiTextureType_UNKNOWN, 0, &tex) == AI_SUCCESS)
      mat.metallic_roughness_map = tex.C_Str();
    if (ai_mat->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &tex) ==
        AI_SUCCESS)
      mat.ao_map = tex.C_Str();

    aiColor3D emissive;
    if (ai_mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS)
      mat.emissive = glm::vec3(emissive.r, emissive.g, emissive.b);

    // ? store on MaterialData for shader selection later
    mat.is_pbr = is_pbr;
    materials.push_back(mat);
  }

  Logger::getInstance().Log(LogLevel::Debug,
                            "[MaterialLoader] Extracted " +
                                std::to_string(materials.size()) +
                                " materials from " + model_path);

  return materials;
}

std::vector<MaterialData> MaterialLoader::LoadFromScene(
    const std::string& path) {
  Assimp::Importer importer;

  // Extract base directory and set it as the working path
  std::string base_dir = path.substr(0, path.find_last_of("/\\") + 1);
  Logger::getInstance().Log(
      LogLevel::Debug, "[MaterialLoader] Loading from base_dir: " + base_dir);

  const aiScene* scene = importer.ReadFile(
      path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace);

  if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) ||
      !scene->mRootNode) {
    Logger::getInstance().Log(LogLevel::Error,
                              "[MaterialLoader] Failed to load scene: " + path +
                                  " - " + importer.GetErrorString());
    return {};
  }

  return ExtractMaterials(scene, path);
}

}  // namespace Content