#ifndef FBM_H
#define FBM_H

#include <FastNoise/FastNoise.h>
#include <glm/glm.hpp>

#include "../noise/OpenSimplex2S.hpp"

class OpenSimplex2S;

namespace Generator {
struct UberFBMParams {
  int octaves = 6;
  float lacunarity = 2.0f;
  float gain = 0.5f;

  float sharpness = 0.0f;         // [-1, 1]: -1=ridge, 0=normal, 1=billow
  float amplify_features = 0.0f;  // Boost certain octaves
  float altitude_erosion = 0.1f;  // Flatten high areas
  float ridge_erosion = 1.0f;     // Sharpen ridges
  float slope_erosion = 0.2f;     // Detail based on steepness
  float perturb = 0.5f;           // Per-octave perturbation strength

  float plateau_threshold = 0.0f;   // Height above which to flatten
  float plateau_smoothness = 0.1f;  // 0 = hard cut, 1 = no effect

  float terrace_frequency = 0.0f;  // 0 = disabled
  float terrace_smoothness = 0.3f;

  bool enable_second_warp = true;
  float second_warp_strength = 0.3f;

  // Per-octave weight curve
  std::array<float, 8> octave_weights = {
      0.8f,  // Octave 0: Reduce base
      1.0f,  // Octave 1: Major features
      1.3f,  // Octave 2: Peak detail
      1.0f,  // Octave 3: Fine detail
      0.7f,  // Octave 4: Very fine
      0.5f,  // Octave 5
      0.3f,  // Octave 6
      0.15f  // Octave 7
  };
};

struct UberFBMResult {
  float value;
  glm::vec2 derivative;
};

class UberFBM {
public:
  UberFBM(FastNoise::SmartNode<> noise_source, OpenSimplex2S* simplex_deriv)
      : noise_source_(noise_source), simplex_deriv_(simplex_deriv) {}

  UberFBMResult Eval(float x, float y, int seed,
                     const UberFBMParams& params) const;

private:
  FastNoise::SmartNode<> noise_source_;
  OpenSimplex2S* simplex_deriv_;

  // Apply ridge/billow sharpness
  float ApplySharpness(float value, float sharpness) const;
};

}  // namespace Generator

#endif  // FBM_H