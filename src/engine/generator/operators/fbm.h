#ifndef FBM_H
#define FBM_H

#include <FastNoise/FastNoise.h>
#include <glm/glm.hpp>

namespace Generator {
struct UberFBMParams {
  int octaves = 6;
  float lacunarity = 2.0f;
  float gain = 0.5f;

  float sharpness = 0.0f;         // [-1, 1]: -1=ridge, 0=normal, 1=billow
  float amplify_features = 0.0f;  // Boost certain octaves
  float altitude_erosion = 0.0f;  // Flatten high areas
  float ridge_erosion = 0.0f;     // Sharpen ridges
  float slope_erosion = 0.0f;     // Detail based on steepness
  float perturb = 0.5f;           // Per-octave perturbation strength
};

struct UberFBMResult {
  float value;
  glm::vec2 derivative;
};
class UberFBM {
public:
  UberFBM(FastNoise::SmartNode<> noise_source) : noise_source_(noise_source) {}

  UberFBMResult Eval(float x, float y, int seed,
                     const UberFBMParams& params) const;

private:
  FastNoise::SmartNode<> noise_source_;

  // Apply ridge/billow sharpness
  float ApplySharpness(float value, float sharpness) const;
};

}  // namespace Generator

#endif  // FBM_H