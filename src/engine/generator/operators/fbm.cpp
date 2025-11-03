#include "fbm.h"

#include <algorithm>

namespace Generator {

float UberFBM::ApplySharpness(float value, float sharpness) const {
  // Sharpness blending
  float billow = std::abs(value);
  float ridged = 1.0f - std::abs(value);

  // Normalize sharpness from [-1, 1] to [0, 1]
  // -1 = full ridge, 0 = original, +1 = full billow
  float t = (sharpness + 1.0f) * 0.5f;
  t = std::clamp(t, 0.0f, 1.0f);

  // Lerp: ridge -> original -> billow
  if (sharpness < 0.0f) {
    // Blend between ridge and original
    return ridged * (-sharpness) + value * (1.0f + sharpness);
  } else {
    // Blend between original and billow
    return value * (1.0f - sharpness) + billow * sharpness;
  }
}

UberFBMResult UberFBM::Eval(float x, float y, int seed,
                            const UberFBMParams& params) const {
  UberFBMResult result;
  result.value = 0.0f;
  result.derivative = glm::vec2(0.0f);

  float frequency = 1.0f;
  float amplitude = 1.0f;

  // Per-octave accumulators
  glm::vec2 perturb_offset(0.0f);
  glm::vec2 slope_erosion_deriv_sum(0.0f);
  glm::vec2 ridge_erosion_deriv_sum(0.0f);

  for (int i = 0; i < params.octaves; ++i) {
    // Apply accumulated domain perturbation
    float perturbed_x = x + perturb_offset.x;
    float perturbed_y = y + perturb_offset.y;

    // Sample noise at perturbed position
    float octave_value = noise_source_->GenSingle2D(
        perturbed_x * frequency, perturbed_y * frequency, seed);

    // Central difference with frequency-scaled step
    const float h =
        0.001f / frequency;  // Scale step size inversely with frequency

    // Central difference: (f(x+h) - f(x-h)) / 2h
    float x_plus = noise_source_->GenSingle2D((perturbed_x + h) * frequency,
                                              perturbed_y * frequency, seed);
    float x_minus = noise_source_->GenSingle2D((perturbed_x - h) * frequency,
                                               perturbed_y * frequency, seed);

    float y_plus = noise_source_->GenSingle2D(
        perturbed_x * frequency, (perturbed_y + h) * frequency, seed);
    float y_minus = noise_source_->GenSingle2D(
        perturbed_x * frequency, (perturbed_y - h) * frequency, seed);

    glm::vec2 octave_deriv((x_plus - x_minus) / (2.0f * h),
                           (y_plus - y_minus) / (2.0f * h));

    // Apply sharpness
    octave_value = ApplySharpness(octave_value, params.sharpness);

    // Ridge erosion sharpening
    if (params.ridge_erosion > 0.0f) {
      float ridge_magnitude = glm::length(ridge_erosion_deriv_sum);
      ridge_magnitude = std::clamp(ridge_magnitude, 0.0f, 3.0f);
      float ridge_sharpness = params.ridge_erosion * ridge_magnitude * 0.1f;

      // Blend toward ridged noise based on accumulated derivative
      float ridged = 1.0f - std::abs(octave_value);
      octave_value = std::lerp(octave_value, ridged, ridge_sharpness);
    }

    // Domain perturbation (only first 3 octaves)
    if (params.perturb > 0.0f && i < 3) {
      float perturb_scale = params.perturb * 0.01f / (1.0f + i * 0.5f);
      perturb_offset += octave_deriv * perturb_scale;

      perturb_offset.x = std::clamp(perturb_offset.x, -2.0f, 2.0f);
      perturb_offset.y = std::clamp(perturb_offset.y, -2.0f, 2.0f);
    }

    // Erosion accumulation with per-octave dampening
    float erosion_dampen = 1.0f / (1.0f + i * 0.3f);
    slope_erosion_deriv_sum +=
        octave_deriv * params.slope_erosion * 0.02f * erosion_dampen;
    ridge_erosion_deriv_sum +=
        octave_deriv * params.ridge_erosion * 0.02f * erosion_dampen;

    // Amplitude damping
    float damped_amplitude = amplitude;

    // Slope attenuation
    float attenuation = 1.0f;
    if (params.slope_erosion > 0.0f) {
      float slope_magnitude = glm::length(slope_erosion_deriv_sum);
      slope_magnitude =
          std::clamp(slope_magnitude, 0.0f, 10.0f);  // Safety clamp
      attenuation = 1.0f / (1.0f + slope_magnitude * 0.1f);
    }

    // Per-octave gain modulation
    float octave_weight = 1.0f;
    if (i == 0) {
      // First octave: significantly reduce
      octave_weight = 0.3f;
    } else if (i >= 1 && i <= 3) {
      // Middle octaves: amplify
      octave_weight = 1.0f + params.amplify_features * 0.5f;
    }

    // Accumulate value and derivative
    float final_amplitude = damped_amplitude * octave_weight;
    result.value += final_amplitude * octave_value * attenuation;
    result.derivative += octave_deriv * final_amplitude;

    // Altitude erosion
    if (params.altitude_erosion > 0.0f) {
      // Normalize result.value to [0, 1] range (assuming noise is ~[-1, 1])
      float normalized = std::clamp(result.value * 0.5f + 0.5f, 0.0f, 1.0f);
      // Smoothstep
      float smoothed = normalized * normalized * (3.0f - 2.0f * normalized);
      // Lerp between standard gain and altitude-damped gain
      float erosion_factor =
          (1.0f - params.altitude_erosion) + smoothed * params.altitude_erosion;
      amplitude *= params.gain * erosion_factor;
    } else {
      amplitude *= params.gain;
    }

    // Next octave
    frequency *= params.lacunarity;
  }

  return result;
}
}  // namespace Generator