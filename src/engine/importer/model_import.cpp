#include <bgfx/bgfx.h>
#include <tiny_gltf.h>
#include <glm/glm.hpp>
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

  bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);
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

  // ─── Parse vertex positions ─────────────────
  const auto& pos_accessor =
      model.accessors[primitive.attributes.at("POSITION")];
  const auto& pos_view = model.bufferViews[pos_accessor.bufferView];
  const auto& pos_buffer = model.buffers[pos_view.buffer];

  const float* pos_data = reinterpret_cast<const float*>(
      &pos_buffer.data[pos_view.byteOffset + pos_accessor.byteOffset]);

  size_t vertex_count = pos_accessor.count;
  std::vector<float> vertices(pos_data, pos_data + vertex_count * 3);

  // ─── Parse indices ──────────────────────────
  const auto& idx_accessor = model.accessors[primitive.indices];
  const auto& idx_view = model.bufferViews[idx_accessor.bufferView];
  const auto& idx_buffer = model.buffers[idx_view.buffer];

  std::vector<uint16_t> indices;
  const void* idx_data =
      &idx_buffer.data[idx_view.byteOffset + idx_accessor.byteOffset];
  if (idx_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
    indices.assign(
        reinterpret_cast<const uint16_t*>(idx_data),
        reinterpret_cast<const uint16_t*>(idx_data) + idx_accessor.count);
  } else {
    Logger::getInstance().Log(LogLevel::Error, "Unsupported index format.");
    return;
  }

  // ─── Create BGFX buffers ────────────────────
  bgfx::VertexBufferHandle vbo = bgfx::createVertexBuffer(
      bgfx::makeRef(vertices.data(), sizeof(float) * vertices.size()),
      GltfVertex::layout);

  bgfx::IndexBufferHandle ibo = bgfx::createIndexBuffer(
      bgfx::makeRef(indices.data(), sizeof(uint16_t) * indices.size()));

  // ─── Create and register GameObject ─────────
  auto obj = std::make_shared<GameObject>(next_id++, "EiffelTower");
  obj->SetBuffers(vbo, ibo);
  obj->SetTransform(glm::mat4(1.0f));
  GameObjectManager::getInstance().AddGameObject(obj);

  Logger::getInstance().Log(LogLevel::Info, "Spawned glTF model into scene.");
}

}  // namespace GltfImport
