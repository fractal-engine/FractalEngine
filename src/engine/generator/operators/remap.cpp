#include "remap.h"

#include <cmath>

namespace Generator {
float Terrace(float value, const TerracingParams& params) {
  float step_size = 1.0f / params.steps;
  float stepped = std::floor(value / step_size) * step_size;

  if (params.smoothness > 0.0f) {
    // Compute blend factor within current step
    float blend = std::fmod(value, step_size) / step_size;

    // Smoothstep for smooth transition between terraces
    blend = blend * blend * (3.0f - 2.0f * blend);

    // Lerp between hard step and smooth step
    return stepped + (step_size * blend * params.smoothness);
  }

  return stepped;
}

float Plateau(float value, const PlateauParams& params) {
  if (value < params.threshold - params.smoothness) {
    return value;  // Below plateau
  }

  if (value > params.threshold + params.smoothness) {
    return params.threshold;  // On plateau
  }

  // Smooth transition zone
  float t = (value - (params.threshold - params.smoothness)) /
            (2.0f * params.smoothness);
  t = t * t * (3.0f - 2.0f * t);  // Smoothstep
  return std::lerp(value, params.threshold, t);
}
}  // namespace Generator
