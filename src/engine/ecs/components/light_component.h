#ifndef LIGHT_COMPONENT_H
#define LIGHT_COMPONENT_H

#include <cstdint>
#include <glm/glm.hpp>

enum class ShadowResolution : uint16_t {
  NONE = 0,
  LOW = 1024,
  MEDIUM = 2048,
  HIGH = 4096
};

// Directional light: Direction comes from Transform's forward vector
struct DirectionalLightComponent {
  bool enabled_ = true;
  glm::vec3 color_ = glm::vec3(1.0f);
  float intensity_ = 5.0f;  // radiance scale

  // Shadows
  // light-specific parameters
  bool cast_shadows_ = true;
  float shadow_bias_ = 0.0005f;
  float normal_bias_ = 0.03f;
  ShadowResolution shadow_resolution_ = ShadowResolution::MEDIUM;
  float shadow_softness_ = 1.0f;  // PCF radius in texels
};

// Point light: position comes from Transform.position
struct PointLightComponent {
  bool enabled_ = true;
  glm::vec3 color_ = glm::vec3(1.0f);
  float intensity_ = 1.0f;
  float range_ = 10.0f;  // meters
  float falloff_ = 5.0f;

  bool cast_shadows_ = false;
  float shadow_bias_ = 0.0005f;
  ShadowResolution shadow_resolution_ = ShadowResolution::LOW;
  float shadow_softness_ = 1.0f;
};

// Spot light: position+direction come from Transform
struct SpotlightComponent {
  bool enabled_ = true;
  glm::vec3 color_ = glm::vec3(1.0f);
  float intensity_ = 1.0f;
  float range_ = 10.0f;
  float falloff_ = 5.0f;
  float inner_angle_ = 45.0f;  // degrees
  float outer_angle_ = 60.0f;  // degrees

  bool cast_shadows_ = false;
  float shadow_bias_ = 0.0005f;
  ShadowResolution shadow_resolution_ = ShadowResolution::LOW;
  float shadow_softness_ = 1.0f;
};

struct SkyLightComponent {
  bool enabled_ = true;
  glm::vec3 color_ = glm::vec3(0.15f, 0.18f, 0.25f);
  float intensity_ = 1.0f;  // ambient radiance scale

  bool cast_shadows_ = true;
  bool real_time_capture_ = false;
  // TODO: source type, cubemap res, cubemap angle, affects world, volumetric..
  // scattering intensity
};

#endif  // LIGHT_COMPONENT_H
