#include "game/game_object.h"
#include <bgfx/bgfx.h>
#include <glm/gtc/type_ptr.hpp>
#include "engine/core/view_ids.h"
#include "engine/importer/gltf_program.h"

/*********************************************************************************
 * @brief Implementation of GameObject class
 * @see GameObject.h
 *
 * Provides:
 *   - Handles for vertex/index/texture rendering
 *   - Static sampler management for all GameObjects
 **********************************************************************************/

// ---------------------------------
// Static Uniform Handle Definitions
// ---------------------------------
bgfx::UniformHandle GameObject::s_diffuseSampler_ = BGFX_INVALID_HANDLE;
bgfx::UniformHandle GameObject::s_ormSampler_ = BGFX_INVALID_HANDLE;
bgfx::UniformHandle GameObject::s_normalSampler_ = BGFX_INVALID_HANDLE;

// ---------------------------------
// Constructor / Destructor
// ---------------------------------
GameObject::GameObject(int id, const std::string& name)
    : id_(id), name_(name) {}

GameObject::~GameObject() {}

// ---------------------------------
// Basic Getters
// ---------------------------------
int GameObject::GetId() const {
  return id_;
}

std::string GameObject::GetName() const {
  return name_;
}

const glm::mat4& GameObject::GetTransform() const {
  return transform_;
}

// ---------------------------------
// Buffer + Transform Setters
// ---------------------------------
void GameObject::SetTransform(const glm::mat4& transform) {
  transform_ = transform;
}

void GameObject::SetBuffers(bgfx::VertexBufferHandle vbo,
                            bgfx::IndexBufferHandle ibo) {
  vbo_ = vbo;
  ibo_ = ibo;
}

// ---------------------------------
// Texture Setters (Per-Instance)
// ---------------------------------
void GameObject::SetTexture(bgfx::TextureHandle texture) {
  texture_ = texture;
}

void GameObject::SetORMTexture(bgfx::TextureHandle texture) {
  ormTexture_ = texture;
}

void GameObject::SetNormalTexture(bgfx::TextureHandle texture) {
  normalTexture_ = texture;
}

// ---------------------------------
// Sampler Getters (Static)
// ---------------------------------
bgfx::UniformHandle GameObject::GetDiffuseSampler() {
  return s_diffuseSampler_;
}

bgfx::UniformHandle GameObject::GetORMSampler() {
  return s_ormSampler_;
}

bgfx::UniformHandle GameObject::GetNormalSampler() {
  return s_normalSampler_;
}

// ---------------------------------
// Sampler Setters (Static)
// ---------------------------------
void GameObject::SetDiffuseSampler(bgfx::UniformHandle handle) {
  s_diffuseSampler_ = handle;
}

void GameObject::SetORMSampler(bgfx::UniformHandle handle) {
  s_ormSampler_ = handle;
}

void GameObject::SetNormalSampler(bgfx::UniformHandle handle) {
  s_normalSampler_ = handle;
}

// ---------------------------------
// Render Logic
// ---------------------------------
void GameObject::Render(const RenderContext& context) {
  if (!bgfx::isValid(vbo_) || !bgfx::isValid(ibo_))
    return;

  // Apply transform matrix
  float mtx[16];
  memcpy(mtx, glm::value_ptr(transform_), sizeof(mtx));
  bgfx::setTransform(mtx);

  // Set geometry buffers
  bgfx::setVertexBuffer(0, vbo_);
  bgfx::setIndexBuffer(ibo_);

  // Bind object-specific textures if valid
  if (bgfx::isValid(texture_)) {
    bgfx::setTexture(0, s_diffuseSampler_, texture_);
  }
  if (bgfx::isValid(ormTexture_)) {
    bgfx::setTexture(1, s_ormSampler_, ormTexture_);
  }
  if (bgfx::isValid(normalTexture_)) {
    bgfx::setTexture(2, s_normalSampler_, normalTexture_);
  }

  // --- NEW: Bind the shared lighting uniforms and textures from the context
  // ---
  bgfx::setUniform(context.u_cameraPos, context.cameraPosValue);
  bgfx::setUniform(context.u_sunDirection, context.sunDirValue);
  bgfx::setUniform(context.u_sunLuminance, context.sunLuminanceValue);
  bgfx::setUniform(context.u_skyAmbient, context.skyAmbientValue);
  bgfx::setUniform(context.u_lightMatrix, context.lightMatrixValue);

  // Bind the shadow map to the slot defined in the shader (fs_gltf.sc)
  bgfx::setTexture(4, context.u_shadowSampler, context.shadowMapTexture);
  // -----------------------------------------------------------------------------

  // Submit draw call using GLTF program
  if (bgfx::isValid(g_gltfProgram)) {
    bgfx::submit(ViewID::SCENE_MESH, g_gltfProgram);
  }
}

