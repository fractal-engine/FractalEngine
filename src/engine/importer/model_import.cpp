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
// Struct for glTF vertex layout (position only for now)
// ─────────────────────────────────────────────
struct GltfVertex {
  float x, y, z;
  static bgfx::VertexLayout layout;
};

bgfx::VertexLayout GltfVertex::layout;

// ─────────────────────────────────────────────
// Call once to initialize the layout
// ─────────────────────────────────────────────
void SetupGltfLayouts() {
  GltfVertex::layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .end();

  Logger::getInstance().Log(LogLevel::Debug,
                            "[GltfImport] Vertex layout initialized.");
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

  // ─── Parse vertex positions ──────────────────────────────
  const auto& pos_accessor =
      model.accessors[primitive.attributes.at("POSITION")];
  const auto& pos_view = model.bufferViews[pos_accessor.bufferView];
  const auto& pos_buffer = model.buffers[pos_view.buffer];

  const float* pos_data = reinterpret_cast<const float*>(
      &pos_buffer.data[pos_view.byteOffset + pos_accessor.byteOffset]);

  size_t vertex_count = pos_accessor.count;
  std::vector<float> vertices(pos_data, pos_data + vertex_count * 3);

  // ─── Parse indices ────────────────────────────────────────
  const auto& idx_accessor = model.accessors[primitive.indices];
  const auto& idx_view = model.bufferViews[idx_accessor.bufferView];
  const auto& idx_buffer = model.buffers[idx_view.buffer];

  const void* idx_data =
      &idx_buffer.data[idx_view.byteOffset + idx_accessor.byteOffset];
  size_t index_count = idx_accessor.count;

  bgfx::IndexBufferHandle ibo = BGFX_INVALID_HANDLE;

  // Index handling based on component type
  if (idx_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
    using IndexType = uint16_t;
    const IndexType* src = reinterpret_cast<const IndexType*>(idx_data);
    const bgfx::Memory* indexMem =
        bgfx::copy(src, sizeof(IndexType) * index_count);
    ibo = bgfx::createIndexBuffer(indexMem, BGFX_BUFFER_NONE);
  } else if (idx_accessor.componentType ==
             TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
    using IndexType = uint32_t;
    const IndexType* src = reinterpret_cast<const IndexType*>(idx_data);
    const bgfx::Memory* indexMem =
        bgfx::copy(src, sizeof(IndexType) * index_count);
    ibo = bgfx::createIndexBuffer(indexMem, BGFX_BUFFER_INDEX32);
  } else {
    Logger::getInstance().Log(LogLevel::Error,
                              "Unsupported index component type.");
    return;
  }

  // ─── Create vertex buffer using bgfx::copy ────────────────
  const bgfx::Memory* vertexMem =
      bgfx::copy(vertices.data(), sizeof(float) * vertices.size());
  bgfx::VertexBufferHandle vbo =
      bgfx::createVertexBuffer(vertexMem, GltfVertex::layout);

  // ─── Create and register GameObject ───────────────────────
  auto obj = std::make_shared<GameObject>(next_id++, "EiffelTower");
  obj->SetBuffers(vbo, ibo);

  glm::mat4 transform = glm::mat4(1.0f);
  transform =
      glm::translate(transform, glm::vec3(0.0f, 10.0f, 0.0f));  // Lift up
  transform = glm::scale(transform, glm::vec3(250.0f));           // Scale up
  obj->SetTransform(transform);

  GameObjectManager::getInstance().AddGameObject(obj);

  Logger::getInstance().Log(LogLevel::Info, "Spawned glTF model into scene.");
  Logger::getInstance().Log(LogLevel::Debug,
                            "Model has " + std::to_string(vertex_count) +
                                " vertices and " + std::to_string(index_count) +
                                " indices.");
}

}  // namespace GltfImport
