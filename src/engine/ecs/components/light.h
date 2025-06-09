#ifndef LIGHT_H
#define LIGHT_H

enum class LightType { Directional, Point, Spot };

struct Light {
  LightType type = LightType::Directional;
  float direction[3] = {0.3f, 1.0f, 0.4f};  // normalized
  float intensity = 1.0f;
};

#endif  // LIGHT_H
