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

// Define the static uniform handle
bgfx::UniformHandle GameObject::GetDiffuseSampler() {
  return s_diffuseSampler_;
}

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
// Assign transform matrix
void GameObject::SetTransform(const glm::mat4& transform) {
  transform_ = transform;
}

// ---------------------------------------------------------------------------------
// Retrieve transform matrix
const glm::mat4& GameObject::GetTransform() const {
  return transform_;
}

// ---------------------------------------------------------------------------------
// Assign vertex and index buffer handles
void GameObject::SetBuffers(bgfx::VertexBufferHandle vbo,
                            bgfx::IndexBufferHandle ibo) {
  vbo_ = vbo;
  ibo_ = ibo;
}

// ---------------------------------------------------------------------------------
// Assign texture handle
void GameObject::SetTexture(bgfx::TextureHandle texture) {
  texture_ = texture;
}

// Define and initialize the static uniform handle
bgfx::UniformHandle GameObject::s_diffuseSampler_ = BGFX_INVALID_HANDLE;

void GameObject::SetDiffuseSampler(bgfx::UniformHandle handle) {
  s_diffuseSampler_ = handle;
}


// ---------------------------------------------------------------------------------
void GameObject::Render() {
  if (!bgfx::isValid(vbo_) || !bgfx::isValid(ibo_))
    return;
  // Apply the transformation matrix
  float mtx[16];
  memcpy(mtx, glm::value_ptr(transform_), sizeof(mtx));
  bgfx::setTransform(mtx);

  bgfx::setVertexBuffer(0, vbo_);
  bgfx::setIndexBuffer(ibo_);

  // Bind texture if available
  if (bgfx::isValid(texture_)) {
    bgfx::setTexture(0, s_diffuseSampler_, texture_);
  }
  // Submit the draw call
  if (bgfx::isValid(g_gltfProgram)) {
    bgfx::submit(ViewID::SCENE_MESH, g_gltfProgram);
  }
}
