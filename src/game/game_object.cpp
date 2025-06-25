#include "game/game_object.h"
#include <bgfx/bgfx.h>
#include <glm/gtc/type_ptr.hpp>  // For glm::value_ptr
#include "engine/core/view_ids.h"
#include "engine/importer/gltf_program.h"

/*********************************************************************************
 * @brief Implementation of GameObject class
 * @see GameObject.h
 *
 * Provides:
 *   - Constructor for assigning ID and Name
 *   - virtual destructor that currently does nothing
 *   - Basic getters for ID and Name
 *
 * Development:
 *   - Tied to GameObject.h, which contains the class definition.
 *   - New methods or attributes should be implemented here, follow
 *     GameObject.h.
 **********************************************************************************/

// Constructor definition
GameObject::GameObject(int id, const std::string& name)
    : id_(id), name_(name) {}

// Destructor definition
GameObject::~GameObject() {}

int GameObject::GetId() const {
  return id_;
}

std::string GameObject::GetName() const {
  return name_;
}

// ---------------------------------------------------------------------------------
// @brief Assign transform matrix
void GameObject::SetTransform(const glm::mat4& transform) {
  transform_ = transform;
}

// ---------------------------------------------------------------------------------
// @brief Retrieve transform matrix
const glm::mat4& GameObject::GetTransform() const {
  return transform_;
}

// ---------------------------------------------------------------------------------
// @brief Assign vertex and index buffer handles
void GameObject::SetBuffers(bgfx::VertexBufferHandle vbo,
                            bgfx::IndexBufferHandle ibo) {
  vbo_ = vbo;
  ibo_ = ibo;
}

// ---------------------------------------------------------------------------------
  void GameObject::Render() {
    if (!bgfx::isValid(vbo_) || !bgfx::isValid(ibo_))
      return;

    // Apply transform
    float mtx[16];
    memcpy(mtx, glm::value_ptr(transform_), sizeof(mtx));
    bgfx::setTransform(mtx);

    // Set buffers
    bgfx::setVertexBuffer(0, vbo_);
    bgfx::setIndexBuffer(ibo_);

    // Submit draw call using the global glTF shader program
    if (bgfx::isValid(g_gltfProgram)) {
      bgfx::submit(ViewID::SCENE_MESH, g_gltfProgram);
    }
  }
