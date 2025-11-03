#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

class ShadowMap {
public:
  ShadowMap(uint16_t resolution = 4096);
  ~ShadowMap();

  void Create();
  void Destroy();

  // Getters
  bgfx::TextureHandle GetTexture() const { return depth_texture_; }
  bgfx::FrameBufferHandle GetFramebuffer() const { return framebuffer_; }
  const glm::mat4& GetLightSpaceMatrix() const { return light_space_matrix_; }
  uint16_t GetResolution() const { return resolution_; }

  // Update light space matrix
  void SetLightSpaceMatrix(const glm::mat4& light_space) {
    light_space_matrix_ = light_space;
  }

  // Uniform getters
  bgfx::UniformHandle GetLightMatrixUniform() const { return u_light_matrix_; }
  bgfx::UniformHandle GetSamplerUniform() const { return s_shadow_map_; }

private:
  uint16_t resolution_;
  bgfx::TextureHandle depth_texture_;
  bgfx::FrameBufferHandle framebuffer_;
  glm::mat4 light_space_matrix_;

  // Uniforms
  bgfx::UniformHandle u_light_matrix_;
  bgfx::UniformHandle s_shadow_map_;
};

#endif  // SHADOW_MAP_H