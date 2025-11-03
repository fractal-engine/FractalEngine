#ifndef SIMPLEX_H
#define SIMPLEX_H

#include <cstdint> 
#include <glm/glm.hpp>

namespace Generator {

// Simplex noise + analytical derivatives
//
struct SimplexResult {
  float value;           // Noise value [-1, 1]
  glm::vec2 derivative;  // Gradient (dx, dy)
};

class SimplexNoise {
public:
  explicit SimplexNoise(uint32_t seed = 0);

  // Standard noise
  float Noise2D(float x, float y) const;

  // Noise with analytical derivatives (critical for domain warping)
  SimplexResult NoiseWithDerivatives(float x, float y) const;

private:
  uint8_t perm_[512];

  // Simplex noise helper functions
  static float Dot(const int* g, float x, float y);
  static int FastFloor(float x);
};

}  // namespace Generator

#endif  // SIMPLEX_H