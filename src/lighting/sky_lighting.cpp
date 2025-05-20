#include "lighting/sky_lighting.h"

// Smooth interpolation for transitions
static float smoothStepRange(float edge0, float edge1, float x) {
  float t = (x - edge0) / (edge1 - edge0);
  t = bx::clamp(t, 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

void SkyLighting::Init() {
  if (!bgfx::isValid(_uSkyAmbient)) {
    _uSkyAmbient = bgfx::createUniform("u_skyAmbient", bgfx::UniformType::Vec4);
  }
}

void SkyLighting::Update(const bx::Vec3& sunDir, float time) {
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
  float brightness =
      bx::clamp(sunH * 0.5f + 0.5f, 0.2f, 1.0f);  // from 0.2 to 1.0
  sky.x *= brightness;
  sky.y *= brightness;
  sky.z *= brightness;

  _ambientColor[0] = sky.x;
  _ambientColor[1] = sky.y;
  _ambientColor[2] = sky.z;
  _ambientColor[3] = 1.0f;
}

void SkyLighting::ApplyUniforms() {
  bgfx::setUniform(_uSkyAmbient, _ambientColor);
}

bgfx::UniformHandle SkyLighting::GetAmbientUniform() const {
  return _uSkyAmbient;
}
