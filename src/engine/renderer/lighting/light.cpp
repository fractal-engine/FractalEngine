#include "light.h"

namespace Light {

namespace {
bgfx::UniformHandle uSkyAmbient = BGFX_INVALID_HANDLE;
bgfx::UniformHandle uSunDirection = BGFX_INVALID_HANDLE;
bgfx::UniformHandle uSunLuminance = BGFX_INVALID_HANDLE;

float ambientColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
float sunDirection[4] = {0.0f, -1.0f, 0.0f, 0.0f};
float sunLuminance[4] = {1.0f, 1.0f, 1.0f, 1.0f};
}  // namespace

// Smooth interpolation for transitions
static float smoothStepRange(float edge0, float edge1, float x) {
  float t = (x - edge0) / (edge1 - edge0);
  t = bx::clamp(t, 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

void Create() {
  if (!bgfx::isValid(uSkyAmbient)) {
    uSkyAmbient = bgfx::createUniform("u_skyAmbient", bgfx::UniformType::Vec4);
  }
  if (!bgfx::isValid(uSunDirection)) {
    uSunDirection =
        bgfx::createUniform("u_sunDirection", bgfx::UniformType::Vec4);
  }
  if (!bgfx::isValid(uSunLuminance)) {
    uSunLuminance =
        bgfx::createUniform("u_sunLuminance", bgfx::UniformType::Vec4);
  }
}

void Destroy() {
  if (bgfx::isValid(uSkyAmbient)) {
    bgfx::destroy(uSkyAmbient);
    uSkyAmbient = BGFX_INVALID_HANDLE;
  }
  if (bgfx::isValid(uSunDirection)) {
    bgfx::destroy(uSunDirection);
    uSunDirection = BGFX_INVALID_HANDLE;
  }
  if (bgfx::isValid(uSunLuminance)) {
    bgfx::destroy(uSunLuminance);
    uSunLuminance = BGFX_INVALID_HANDLE;
  }
}

void SetDirectionalLight(const glm::vec3& direction, const glm::vec3& color,
                         float intensity) {
  sunDirection[0] = direction.x;
  sunDirection[1] = direction.y;
  sunDirection[2] = direction.z;
  sunDirection[3] = 0.0f;

  sunLuminance[0] = color.r * intensity;
  sunLuminance[1] = color.g * intensity;
  sunLuminance[2] = color.b * intensity;
  sunLuminance[3] = intensity;
}

void UpdateSkyAmbient(const bx::Vec3& sunDir, float time) {
  const float sunH = sunDir.y;

  // Day-night blend factor based on sun height
  const float dayNight = smoothStepRange(-0.15f, 0.2f, sunH);

  // Sunrise/sunset boost factor
  float sunrise = smoothStepRange(0.0f, 0.2f, sunH) *
                  (1.0f - smoothStepRange(0.1f, 0.3f, sunH));
  sunrise = bx::min(sunrise * 3.0f, 1.0f);  // boost

  // Ambient sky colors
  bx::Vec3 night = {0.03f, 0.04f, 0.12f};
  bx::Vec3 day = {0.50f, 0.70f, 0.95f};
  bx::Vec3 sunset = {0.9f, 0.4f, 0.15f};

  // Blend day/night
  bx::Vec3 sky = {bx::lerp(night.x, day.x, dayNight),
                  bx::lerp(night.y, day.y, dayNight),
                  bx::lerp(night.z, day.z, dayNight)};

  // Blend in sunset hues
  sky.x = bx::lerp(sky.x, sunset.x, sunrise);
  sky.y = bx::lerp(sky.y, sunset.y, sunrise);
  sky.z = bx::lerp(sky.z, sunset.z, sunrise);

  // Apply brightness factor based on sun height
  float brightness = bx::clamp(sunH * 0.5f + 0.5f, 0.2f, 1.0f);
  sky.x *= brightness;
  sky.y *= brightness;
  sky.z *= brightness;

  ambientColor[0] = sky.x;
  ambientColor[1] = sky.y;
  ambientColor[2] = sky.z;
  ambientColor[3] = 1.0f;
}

void ApplyUniforms() {
  if (bgfx::isValid(uSkyAmbient)) {
    bgfx::setUniform(uSkyAmbient, ambientColor);
  }
  if (bgfx::isValid(uSunDirection)) {
    bgfx::setUniform(uSunDirection, sunDirection);
  }
  if (bgfx::isValid(uSunLuminance)) {
    bgfx::setUniform(uSunLuminance, sunLuminance);
  }
}

bgfx::UniformHandle GetAmbientUniform() {
  return uSkyAmbient;
}

bgfx::UniformHandle GetSunDirectionUniform() {
  return uSunDirection;
}

bgfx::UniformHandle GetSunLuminanceUniform() {
  return uSunLuminance;
}

}  // namespace Light
