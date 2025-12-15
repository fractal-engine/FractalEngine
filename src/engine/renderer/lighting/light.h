#ifndef LIGHT_H
#define LIGHT_H

#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <glm/glm.hpp>

namespace Light {
void Create();
void Destroy();

void SetDirectionalLight(const glm::vec3& direction, const glm::vec3& color,
                         float intensity);

void UpdateSkyAmbient(const bx::Vec3& sunDir, float time);

void ApplyUniforms();

// Getters
bgfx::UniformHandle GetAmbientUniform();
bgfx::UniformHandle GetSunDirectionUniform();
bgfx::UniformHandle GetSunLuminanceUniform();
}  // namespace Light

#endif  // LIGHT_H
