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

  // Warped base coordinates
  float base_x = x;
  float base_y = y;

  // Per-octave accumulators
  glm::vec2 perturb_offset(0.0f);
  glm::vec2 slope_erosion_deriv_sum(0.0f);
  glm::vec2 ridge_erosion_deriv_sum(0.0f);
  glm::vec2 ridge_sharpness_accum(0.0f);

  // Second domain warp
  if (params.enable_second_warp) {
    /**
     * ? Uses custom OpenSimplex2S instead of FastNoise2 to avoid grid artifacts
     */

    // Sample noise at two different offsets (q vector)
    double q_x_val, q_x_dx, q_x_dy;
    simplex_deriv_->noise2_deriv(x, y, q_x_val, q_x_dx, q_x_dy);

    double q_y_val, q_y_dx, q_y_dy;
    simplex_deriv_->noise2_deriv(x + 5.2, y + 1.3, q_y_val, q_y_dx, q_y_dy);

    float q_x = (float)q_x_val;
    float q_y = (float)q_y_val;

    // Apply first warp to get r vector
    double r_x_val, r_x_dx, r_x_dy;
    simplex_deriv_->noise2_deriv((x + 4.0 * q_x) + 1.7, (y + 4.0 * q_y) + 9.2,
                                 r_x_val, r_x_dx, r_x_dy);

    double r_y_val, r_y_dx, r_y_dy;
    simplex_deriv_->noise2_deriv((x + 4.0 * q_x) + 8.3, (y + 4.0 * q_y) + 2.8,
                                 r_y_val, r_y_dx, r_y_dy);

    float r_x = (float)r_x_val;
    float r_y = (float)r_y_val;

    // Apply warp to base coordinates
    base_x += r_x * params.second_warp_strength * 0.2f;
    base_y += r_y * params.second_warp_strength * 0.2f;
  }

  for (int i = 0; i < params.octaves; ++i) {
    // Apply accumulated domain perturbation
    float perturbed_x = base_x + perturb_offset.x;
    float perturbed_y = base_y + perturb_offset.y;

    // ANALYTICAL DERIVATIVES
    double noise_value, noise_dx, noise_dy;
    simplex_deriv_->noise2_deriv(perturbed_x * frequency,
                                 perturbed_y * frequency,
                                 noise_value,  // output
                                 noise_dx,     // output
                                 noise_dy);    // output

    float octave_value = (float)noise_value;
    glm::vec2 octave_deriv((float)noise_dx * frequency,  // Scale by frequency
                           (float)noise_dy * frequency);

    // Apply sharpness
    octave_value = ApplySharpness(octave_value, params.sharpness);

    // Apply plateau per-octave
    if (params.plateau_threshold > 0.0f) {
      if (octave_value > params.plateau_threshold) {
        float overshoot = octave_value - params.plateau_threshold;
        float dampened = overshoot * params.plateau_smoothness;
        octave_value = params.plateau_threshold + dampened;
      }
    }

    // Apply terrace per-octave
    if (params.terrace_frequency > 0.0f) {
      float step_size = 1.0f / params.terrace_frequency;
      float stepped = std::floor(octave_value / step_size) * step_size;

      if (params.terrace_smoothness > 0.0f) {
        float blend = std::fmod(octave_value, step_size) / step_size;
        blend = blend * blend * (3.0f - 2.0f * blend);  // Smoothstep
        octave_value =
            stepped + (step_size * blend * params.terrace_smoothness);
      } else {
        octave_value = stepped;
      }
    }

    // Cumulative ridge sharpening
    ridge_sharpness_accum += octave_deriv * params.ridge_erosion * 0.02f;

    // Ridge sharpening
    if (params.ridge_erosion > 0.0f && i > 0) {
      float ridge_strength = glm::length(ridge_sharpness_accum);
      ridge_strength = std::clamp(ridge_strength, 0.0f, 1.0f);

      float ridged = 1.0f - std::abs(octave_value);
      octave_value = std::lerp(octave_value, ridged, ridge_strength);
    }

    // Domain perturbation (only first 3 octaves)
    if (params.perturb > 0.0f && i < 3) {
      float perturb_scale = params.perturb * 0.05f / (1.0f + i * 0.5f);
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

    // Apply per-octave weight
    float octave_weight = (i < 8) ? params.octave_weights[i] : 1.0f;

    // Global amplification modulates the weight curve
    octave_weight *= (1.0f + params.amplify_features * 0.5f);

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
