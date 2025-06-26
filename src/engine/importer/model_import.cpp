#include <bgfx/bgfx.h>
#include <tiny_gltf.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include "editor/runtime/application.h"
#include "engine/core/logger.h"
#include "engine/core/view_ids.h"
#include "engine/importer/gltf_program.h"
#include "engine/resources/shader_utils.h"
#include "game/game_object_manager.h"

namespace GltfImport {

// ─────────────────────────────────────────────
// Struct for glTF vertex layout
// ─────────────────────────────────────────────
struct GltfVertex {
  float x, y, z;  // Position
  float u, v;     // Texture coordinates
  static bgfx::VertexLayout layout;
};

bgfx::VertexLayout GltfVertex::layout;

// ─────────────────────────────────────────────
// Call once to initialize the layout
// ─────────────────────────────────────────────
void SetupGltfLayouts() {
  GltfVertex::layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .end();

  Logger::getInstance().Log(LogLevel::Debug,
                            "[GltfImport] Vertex layout with UVs initialized.");
}

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
// ─────────────────────────────────────────────
// Import glTF file and spawn a GameObject
// ─────────────────────────────────────────────
void LoadModelAndSpawn(const std::string& filepath) {
  static int next_id = 1;

  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err, warn;

  bool ret = false;
  if (filepath.size() >= 4 && filepath.substr(filepath.size() - 4) == ".glb") {
    ret = loader.LoadBinaryFromFile(&model, &err, &warn, filepath);
  } else {
    ret = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);
  }

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

  // ─── Vertex Attribute Parsing ─────────────────────────────
  const auto& pos_accessor =
      model.accessors[primitive.attributes.at("POSITION")];
  const auto& pos_view = model.bufferViews[pos_accessor.bufferView];
  const auto& pos_buffer = model.buffers[pos_view.buffer];
  const float* pos_data = reinterpret_cast<const float*>(
      &pos_buffer.data[pos_view.byteOffset + pos_accessor.byteOffset]);
  size_t vertex_count = pos_accessor.count;

  // Optional: UV Parsing (if present)
  std::vector<GltfVertex> vertexBuffer;
  bool hasTexcoords =
      primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end();

  const float* texcoord_data = nullptr;
  if (hasTexcoords) {
    const auto& uv_accessor =
        model.accessors[primitive.attributes.at("TEXCOORD_0")];
    const auto& uv_view = model.bufferViews[uv_accessor.bufferView];
    const auto& uv_buffer = model.buffers[uv_view.buffer];
    texcoord_data = reinterpret_cast<const float*>(
        &uv_buffer.data[uv_view.byteOffset + uv_accessor.byteOffset]);
  }

  for (size_t i = 0; i < vertex_count; ++i) {
    GltfVertex v;
    v.x = pos_data[i * 3 + 0];
    v.y = pos_data[i * 3 + 1];
    v.z = pos_data[i * 3 + 2];
    v.u = hasTexcoords ? texcoord_data[i * 2 + 0] : 0.0f;
    v.v = hasTexcoords ? texcoord_data[i * 2 + 1] : 0.0f;
    vertexBuffer.push_back(v);
  }

  // ─── Index Buffer ─────────────────────────────────────────
  const auto& idx_accessor = model.accessors[primitive.indices];
  const auto& idx_view = model.bufferViews[idx_accessor.bufferView];
  const auto& idx_buffer = model.buffers[idx_view.buffer];
  const void* idx_data =
      &idx_buffer.data[idx_view.byteOffset + idx_accessor.byteOffset];
  size_t index_count = idx_accessor.count;

  bgfx::IndexBufferHandle ibo = BGFX_INVALID_HANDLE;

  if (idx_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
    const uint16_t* src = reinterpret_cast<const uint16_t*>(idx_data);
    const bgfx::Memory* indexMem =
        bgfx::copy(src, sizeof(uint16_t) * index_count);
    ibo = bgfx::createIndexBuffer(indexMem, BGFX_BUFFER_NONE);
  } else if (idx_accessor.componentType ==
             TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
    const uint32_t* src = reinterpret_cast<const uint32_t*>(idx_data);
    const bgfx::Memory* indexMem =
        bgfx::copy(src, sizeof(uint32_t) * index_count);
    ibo = bgfx::createIndexBuffer(indexMem, BGFX_BUFFER_INDEX32);
  } else {
    Logger::getInstance().Log(LogLevel::Error,
                              "Unsupported index component type.");
    return;
  }

  // ─── Vertex Buffer ────────────────────────────────────────
  const bgfx::Memory* vertexMem =
      bgfx::copy(vertexBuffer.data(), sizeof(GltfVertex) * vertex_count);
  bgfx::VertexBufferHandle vbo =
      bgfx::createVertexBuffer(vertexMem, GltfVertex::layout);

  // ─── Texture (Embedded baseColor) ─────────────────────────
  bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;

  if (primitive.material >= 0 && primitive.material < model.materials.size()) {
    const auto& mat = model.materials[primitive.material];
    if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
      int texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
      const auto& tex = model.textures[texIndex];
      const auto& img = model.images[tex.source];

      if (!img.image.empty()) {
        const bgfx::Memory* texMem =
            bgfx::copy(img.image.data(), (uint32_t)img.image.size());

        texture = bgfx::createTexture2D((uint16_t)img.width,
                                        (uint16_t)img.height, false, 1,
                                        bgfx::TextureFormat::RGBA8, 0, texMem);

        Logger::getInstance().Log(LogLevel::Debug,
                                  "Embedded texture loaded successfully.");
      }
    }
  }

  // ─── Register GameObject ──────────────────────────────────
  auto obj = std::make_shared<GameObject>(next_id++, "EiffelTower");
  obj->SetBuffers(vbo, ibo);
  if (bgfx::isValid(texture)) {
    obj->SetTexture(texture);
  }

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
