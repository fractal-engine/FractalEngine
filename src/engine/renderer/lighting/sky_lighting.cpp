#include "sky_lighting.h"

// Helper function for cubic Hermite ease-in/ease-out function
static float smoothStepRange(float edge0, float edge1, float x) {
  // remap x from [edge0, edge1] into [0,1]
  float t = (x - edge0) / (edge1 - edge0);
  // clamp into 0 to 1
  t = bx::clamp(t, 0.0f, 1.0f);
  // classic smoothstep curve
  return t * t * (3.0f - 2.0f * t);
}
void SkyLighting::Init() {
  if (!bgfx::isValid(_uSkyAmbient)) {
    _uSkyAmbient = bgfx::createUniform("u_skyAmbient", bgfx::UniformType::Vec4);
  }
}

void SkyLighting::Update(const bx::Vec3& sunDir, float time) {
  const float sunH = sunDir.y;

  const float dayNight = smoothStepRange(-0.15f, 0.2f, sunH);
  float sunrise = smoothStepRange(0.0f, 0.2f, sunH)
                  *(1.0f - smoothStepRange(0.1f, 0.3f, sunH));
  sunrise = bx::min(sunrise * 3.0f, 1.0f);

  bx::Vec3 night = {0.03f, 0.04f, 0.12f};
  bx::Vec3 day = {0.50f, 0.70f, 0.95f};
  bx::Vec3 sunset = {0.9f, 0.4f, 0.15f};

  bx::Vec3 sky = {bx::lerp(night.x, day.x, dayNight),
                  bx::lerp(night.y, day.y, dayNight),
                  bx::lerp(night.z, day.z, dayNight)};

  sky.x = bx::lerp(sky.x, sunset.x, sunrise);
  sky.y = bx::lerp(sky.y, sunset.y, sunrise);
  sky.z = bx::lerp(sky.z, sunset.z, sunrise);

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
