#ifndef FEATURE_DESCRIPTORS_H
#define FEATURE_DESCRIPTORS_H

#include <glm/glm.hpp>

namespace PCG {

struct Feature {
  glm::vec2 center;  // world position
  float radius;      // Influence radius
  float strength;    // Blend strength

  // Parameters
  float frequency;
  float amplitude;
  int octaves;
  float sharpness;
};

// Preset configurations
namespace Descriptors {

// Flat plains
inline Feature Plains(glm::vec2 pos, float radius = 100.0f) {
  Feature param;
  param.center = pos;
  param.radius = radius;
  param.strength = 0.6;
  param.frequency = 0.03f;
  param.amplitude = 15.0f;
  param.octaves = 2;
  param.sharpness = 0.0f;  // Smooth
  return param;
}

// Mountain ranges
inline Feature Mountains(glm::vec2 pos, float radius = 80.0f) {
  Feature param;
  param.center = pos;
  param.radius = radius;
  param.strength = 1.0;
  param.frequency = 0.015f;
  param.amplitude = 60.0f;  // High peaks
  param.octaves = 6;
  param.sharpness = 0.8f;  // Sharp ridges
  return param;
}

inline Feature Valleys(glm::vec2 pos, float radius = 120.0f) {
  Feature param;
  param.center = pos;
  param.radius = radius;
  param.strength = 0.8;
  param.frequency = 0.02f;
  param.amplitude = -40.0f;
  param.octaves = 4;
  param.sharpness = 0.3f;  // Very sharp ridges -> inverted = canyons?
  return param;
}

inline Feature Hills(glm::vec2 pos, float radius = 200.0f) {
  Feature param;
  param.center = pos;
  param.radius = radius;
  param.strength = 0.8;
  param.frequency = 0.05f;
  param.amplitude = 25.0f;
  param.octaves = 4;
  param.sharpness = 0.2f;
  return param;
}

}  // namespace Descriptors
}  // namespace PCG

#endif  // FEATURE_DESCRIPTORS_H