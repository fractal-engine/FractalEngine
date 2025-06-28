#include <bgfx/bgfx.h>
#include <tiny_gltf.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>
#include "editor/runtime/application.h"
#include "engine/core/logger.h"
#include "engine/core/view_ids.h"
#include "engine/importer/gltf_program.h"
#include "engine/resources/shader_utils.h"
#include "game/game_object_manager.h"

namespace GltfImport {

// ------------------------------------------------------------------------------------------
// Struct for glTF vertex layout
// ------------------------------------------------------------------------------------------
struct GltfVertex {
  glm::vec3 position;
  glm::vec2 uv;
  glm::vec3 normal;
  glm::vec4 tangent;
  static bgfx::VertexLayout layout;
};

bgfx::VertexLayout GltfVertex::layout;

// ------------------------------------------------------------------------------------------
// Initialize the vertex layout
// ------------------------------------------------------------------------------------------
void SetupGltfLayouts() {
  GltfVertex::layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Tangent, 4, bgfx::AttribType::Float)
      .end();

  Logger::getInstance().Log(LogLevel::Debug,
                            "[GltfImport] Vertex layout with TBN initialized.");
}

// ------------------------------------------------------------------------------------------
// Load and compile glTF shader program
// ------------------------------------------------------------------------------------------
void SetupGltfProgram() {
  ShaderManager& shaderMgr = *Application::GetShaderManager();
  g_gltfProgram =
      shaderMgr.LoadProgram("gltf_default", "vs_gltf.bin", "fs_gltf.bin");

  if (bgfx::isValid(g_gltfProgram)) {
    Logger::getInstance().Log(LogLevel::Debug, "Loaded glTF shader program.");
  } else {
    Logger::getInstance().Log(LogLevel::Error,
                              "Failed to load glTF shader program.");
  }
}

// ------------------------------------------------------------------------------------------
// Load glTF file and spawn a GameObject
// ------------------------------------------------------------------------------------------
void LoadModelAndSpawn(const std::string& filepath) {
  static int next_id = 1;

  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err, warn;

  bool ret = (filepath.ends_with(".glb"))
                 ? loader.LoadBinaryFromFile(&model, &err, &warn, filepath)
                 : loader.LoadASCIIFromFile(&model, &err, &warn, filepath);

  if (!warn.empty())
    Logger::getInstance().Log(LogLevel::Warning, warn);
  if (!err.empty())
    Logger::getInstance().Log(LogLevel::Error, err);
  if (!ret) {
    Logger::getInstance().Log(LogLevel::Error, "Failed to load glTF file.");
    return;
  }

  if (model.meshes.empty()) {
    Logger::getInstance().Log(LogLevel::Warning,
                              "No meshes found in glTF model.");
    return;
  }

  const tinygltf::Mesh& mesh = model.meshes[0];
  const tinygltf::Primitive& primitive = mesh.primitives[0];

  // ------ Parse vertex attributes ----------------------------------------------------------
  size_t vertex_count =
      model.accessors[primitive.attributes.at("POSITION")].count;

  const float* pos_data = reinterpret_cast<const float*>(
      &model
           .buffers[model
                        .bufferViews[model
                                         .accessors[primitive.attributes.at(
                                             "POSITION")]
                                         .bufferView]
                        .buffer]
           .data[model
                     .bufferViews
                         [model.accessors[primitive.attributes.at("POSITION")]
                              .bufferView]
                     .byteOffset +
                 model.accessors[primitive.attributes.at("POSITION")]
                     .byteOffset]);

  const float* uv_data = nullptr;
  if (primitive.attributes.contains("TEXCOORD_0")) {
    const auto& uv_accessor =
        model.accessors[primitive.attributes.at("TEXCOORD_0")];
    const auto& uv_view = model.bufferViews[uv_accessor.bufferView];
    const auto& uv_buffer = model.buffers[uv_view.buffer];
    uv_data = reinterpret_cast<const float*>(
        &uv_buffer.data[uv_view.byteOffset + uv_accessor.byteOffset]);
  }

  const float* normal_data = nullptr;
  if (primitive.attributes.contains("NORMAL")) {
    const auto& normal_accessor =
        model.accessors[primitive.attributes.at("NORMAL")];
    const auto& normal_view = model.bufferViews[normal_accessor.bufferView];
    const auto& normal_buffer = model.buffers[normal_view.buffer];
    normal_data = reinterpret_cast<const float*>(
        &normal_buffer
             .data[normal_view.byteOffset + normal_accessor.byteOffset]);
  }

  const float* tangent_data = nullptr;
  if (primitive.attributes.contains("TANGENT")) {
    const auto& tan_accessor =
        model.accessors[primitive.attributes.at("TANGENT")];
    const auto& tan_view = model.bufferViews[tan_accessor.bufferView];
    const auto& tan_buffer = model.buffers[tan_view.buffer];
    tangent_data = reinterpret_cast<const float*>(
        &tan_buffer.data[tan_view.byteOffset + tan_accessor.byteOffset]);
  }

  std::vector<GltfVertex> vertexBuffer;
  for (size_t i = 0; i < vertex_count; ++i) {
    GltfVertex v;
    v.position = glm::make_vec3(&pos_data[i * 3]);
    v.uv = uv_data ? glm::make_vec2(&uv_data[i * 2]) : glm::vec2(0.0f);
    v.normal = normal_data ? glm::make_vec3(&normal_data[i * 3])
                           : glm::vec3(0.0f, 1.0f, 0.0f);
    v.tangent = tangent_data ? glm::make_vec4(&tangent_data[i * 4])
                             : glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    vertexBuffer.push_back(v);
  }

  // ------ Index Buffer ----------------------------------------------------------------------------------
  const auto& idx_accessor = model.accessors[primitive.indices];
  const auto& idx_view = model.bufferViews[idx_accessor.bufferView];
  const auto& idx_buffer = model.buffers[idx_view.buffer];
  const void* idx_data =
      &idx_buffer.data[idx_view.byteOffset + idx_accessor.byteOffset];
  size_t index_count = idx_accessor.count;

  bgfx::IndexBufferHandle ibo = BGFX_INVALID_HANDLE;
  if (idx_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
    auto src = reinterpret_cast<const uint16_t*>(idx_data);
    ibo = bgfx::createIndexBuffer(
        bgfx::copy(src, sizeof(uint16_t) * index_count), BGFX_BUFFER_NONE);
  } else if (idx_accessor.componentType ==
             TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
    auto src = reinterpret_cast<const uint32_t*>(idx_data);
    ibo = bgfx::createIndexBuffer(
        bgfx::copy(src, sizeof(uint32_t) * index_count), BGFX_BUFFER_INDEX32);
  } else {
    Logger::getInstance().Log(LogLevel::Error,
                              "Unsupported index component type.");
    return;
  }

  // ------ Vertex Buffer --------------------------------------------------------------------------------
  const bgfx::Memory* vertexMem =
      bgfx::copy(vertexBuffer.data(), sizeof(GltfVertex) * vertex_count);
  bgfx::VertexBufferHandle vbo =
      bgfx::createVertexBuffer(vertexMem, GltfVertex::layout);

  // ------ Load BaseColor / ORM / Normal Textures ------------------------------
  bgfx::TextureHandle baseColorTex = BGFX_INVALID_HANDLE;
  bgfx::TextureHandle ormTex = BGFX_INVALID_HANDLE;
  bgfx::TextureHandle normalTex = BGFX_INVALID_HANDLE;

  if (primitive.material >= 0 && primitive.material < model.materials.size()) {
    const auto& mat = model.materials[primitive.material];
    const auto& pbr = mat.pbrMetallicRoughness;

    auto loadTextureFromIndex = [&](int texIndex) -> bgfx::TextureHandle {
      if (texIndex < 0 || texIndex >= model.textures.size())
        return BGFX_INVALID_HANDLE;
      const auto& tex = model.textures[texIndex];
      if (tex.source < 0 || tex.source >= model.images.size())
        return BGFX_INVALID_HANDLE;
      const auto& img = model.images[tex.source];
      if (img.image.empty())
        return BGFX_INVALID_HANDLE;

      const bgfx::Memory* mem =
          bgfx::copy(img.image.data(), (uint32_t)img.image.size());
      return bgfx::createTexture2D((uint16_t)img.width, (uint16_t)img.height,
                                   false, 1, bgfx::TextureFormat::RGBA8, 0,
                                   mem);
    };

    // Load Base Color
    if (pbr.baseColorTexture.index >= 0) {
      baseColorTex = loadTextureFromIndex(pbr.baseColorTexture.index);
      if (bgfx::isValid(baseColorTex)) {
        Logger::getInstance().Log(LogLevel::Debug, "BaseColor texture loaded.");
      } else {
        Logger::getInstance().Log(LogLevel::Warning,
                                  "BaseColor texture missing or invalid.");
      }
    }

    // Load ORM (metallicRoughnessTexture holds ORM packed in glTF)
    if (pbr.metallicRoughnessTexture.index >= 0) {
      ormTex = loadTextureFromIndex(pbr.metallicRoughnessTexture.index);
      if (bgfx::isValid(ormTex)) {
        Logger::getInstance().Log(LogLevel::Debug, "ORM texture loaded.");
      } else {
        Logger::getInstance().Log(LogLevel::Warning,
                                  "ORM texture missing or invalid.");
      }
    } else {
      Logger::getInstance().Log(LogLevel::Warning,
                                "ORM texture not defined in glTF material.");
    }

    // Load Normal Map
    if (mat.normalTexture.index >= 0) {
      normalTex = loadTextureFromIndex(mat.normalTexture.index);
      if (bgfx::isValid(normalTex)) {
        Logger::getInstance().Log(LogLevel::Debug, "Normal texture loaded.");
      } else {
        Logger::getInstance().Log(LogLevel::Warning,
                                  "Normal texture missing or invalid.");
      }
    } else {
      Logger::getInstance().Log(LogLevel::Warning,
                                "Normal texture not defined in glTF material.");
    }
  }


  // ------ Create and Register GameObject -------------------------------------------------
  auto obj = std::make_shared<GameObject>(next_id++, "EiffelTower");
  obj->SetBuffers(vbo, ibo);

  if (bgfx::isValid(baseColorTex))
    obj->SetTexture(baseColorTex);
  if (bgfx::isValid(ormTex))
    obj->SetORMTexture(ormTex);
  if (bgfx::isValid(normalTex))
    obj->SetNormalTexture(normalTex);

  glm::mat4 transform = glm::mat4(1.0f);
  transform = glm::translate(transform, glm::vec3(0.0f, 10.0f, 0.0f));
  transform = glm::scale(transform, glm::vec3(250.0f));
  obj->SetTransform(transform);

  GameObjectManager::getInstance().AddGameObject(obj);

  Logger::getInstance().Log(LogLevel::Info, "Spawned glTF model into scene.");
  Logger::getInstance().Log(LogLevel::Debug,
                            "Model has " + std::to_string(vertex_count) +
                                " vertices and " + std::to_string(index_count) +
                                " indices.");
}

}  // namespace GltfImport
