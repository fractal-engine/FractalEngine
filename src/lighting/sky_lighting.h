#pragma once

#include <bgfx/bgfx.h>
#include <bx/math.h>

struct SkyLighting {
  void Init();
  void Update(const bx::Vec3& sunDirection, float time);
  void ApplyUniforms();

  bgfx::UniformHandle GetAmbientUniform() const;

private:
  bgfx::UniformHandle _uSkyAmbient = BGFX_INVALID_HANDLE;
  float _ambientColor[4];  // rgb = ambient, a = strength (optional)
};
