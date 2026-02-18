#include "terrain_generator.h"

#include "../constraints/biome_presets.h"
#include "../noise/OpenSimplex2S.hpp"
#include "engine/core/types/geometry_data.h"  // ! should be removed

#include <algorithm>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <limits>
#include <string>

//
// TODO:
// - TerrainGenerator should not know about mesh generation specifics
// - should instead use a builder interface using mesh loader to translate it
// outputs raw data: heights, densities, etc from inside heightmap_mesher
// and converts that into MeshData
//

namespace {
float ComputeFalloff(float distance, float radius) {
  if (distance >= radius)
    return 0.0f;
  float t = 1.0f - (distance / radius);
  return t * t * (3.0f - 2.0f * t);
}
}  // namespace

namespace PCG {

// Constructor
Generator::Generator(const Config& config) : config_(config) {

  // ============================================
  // UBER NOISE PIPELINE (Node Graph)
  // ============================================

  // OUTER LAYER: Domain Warp only
  // Step 1: Base noise source (Simplex/OpenSimplex2)
  // auto simplex = FastNoise::New<FastNoise::OpenSimplex2>();

  // Step 2: FBM (multi-octave fractal)
  /* auto fbm = FastNoise::New<FastNoise::FractalFBm>();
  fbm->SetSource(simplex);
  fbm->SetOctaveCount(config.octaves);
  fbm->SetLacunarity(config.lacunarity);
  fbm->SetGain(config.gain);*/

  // Step 3: Domain Warp FRACTAL (multi-octave warp)
  // Applies warp progressively through octaves
  /* auto warp_gradient = FastNoise::New<FastNoise::DomainWarpGradient>();
  warp_gradient->SetSource(fbm);
  warp_gradient->SetWarpAmplitude(config.perturb);  // Warp strength
  warp_gradient->SetWarpFrequency(config.frequency *
                                  0.5f);  // Lower freq for warp*/

  auto warp_gradient = FastNoise::New<FastNoise::DomainWarpGradient>();
  // warp_gradient->SetSource(simplex);
  warp_gradient->SetWarpAmplitude(config.perturb);
  // warp_gradient->SetWarpFrequency(config.frequency * 0.5f);

  // Step 3: Wrap in DomainWarpFractalProgressive for multi-octave warp
  auto domain_warp_fractal =
      FastNoise::New<FastNoise::DomainWarpFractalProgressive>();
  domain_warp_fractal->SetSource(warp_gradient);

  // Configure fractal parameters (from Fractal<> base)
  domain_warp_fractal->SetOctaveCount(3);
  domain_warp_fractal->SetGain(0.6f);
  domain_warp_fractal->SetLacunarity(2.0f);

  // Store final pipeline
  noise_pipeline_ = domain_warp_fractal;

  // Create OpenSimplex2 with derivatives
  simplex_deriv_ = std::make_unique<OpenSimplex2S>(config.seed);

  // fBm evaluator
  uber_fbm_ =
      std::make_unique<UberFBM>(domain_warp_fractal, simplex_deriv_.get());
}

void Generator::UpdateConfig(const Config& config) {
  config_ = config;
}

Sample Generator::Eval(float x, float y) const {
  Sample result;
  result.curvature = 0.0f;  // Stub for now

  // Set base parameters
  float local_sharpness = config_.sharpness;
  float local_perturb = config_.perturb;
  float local_amplify = config_.amplify;

  // Build Uber FBM params from config
  UberFBMParams uber_params;
  uber_params.octaves = config_.octaves;
  uber_params.lacunarity = config_.lacunarity;
  uber_params.gain = config_.gain;
  uber_params.sharpness = config_.sharpness;
  uber_params.amplify_features = config_.amplify;
  uber_params.altitude_erosion = config_.altitude_erosion;
  uber_params.ridge_erosion = config_.ridge_erosion;
  uber_params.slope_erosion = config_.slope_erosion;
  uber_params.perturb = config_.perturb;

  // ============================================
  // Spatially vary parameters
  // ============================================
  // Vary sharpness: FULL RANGE [-1, 1]
  if (config_.vary_sharpness.enabled) {
    float sharpness_noise = noise_pipeline_->GenSingle2D(
        x * config_.vary_sharpness.frequency,
        y * config_.vary_sharpness.frequency, config_.seed + 1000);

    // Modulate base sharpness
    uber_params.sharpness += sharpness_noise * config_.vary_sharpness.amplitude;
    uber_params.sharpness = std::clamp(uber_params.sharpness, -1.0f, 1.0f);
  }

  // Vary perturbation (if enabled)
  if (config_.vary_perturb.enabled) {
    float perturb_noise = noise_pipeline_->GenSingle2D(
        x * config_.vary_perturb.frequency, y * config_.vary_perturb.frequency,
        config_.seed + 2000);

    uber_params.perturb += perturb_noise * config_.vary_perturb.amplitude;
    uber_params.perturb = std::clamp(uber_params.perturb, 0.0f, 2.0f);
  }

  // Vary feature amplification (if enabled)
  if (config_.vary_amplify.enabled) {
    float amplify_noise = noise_pipeline_->GenSingle2D(
        x * config_.vary_amplify.frequency, y * config_.vary_amplify.frequency,
        config_.seed + 3000);

    uber_params.amplify_features +=
        amplify_noise * config_.vary_amplify.amplitude;
    uber_params.amplify_features =
        std::clamp(uber_params.amplify_features, 0.0f, 2.0f);
  }

  // ============================================
  // EVALUATE UBER NOISE PIPELINE
  // ============================================
  // If debug active: use staged evaluation
  if (config_.debug_stage != PipelineStage::Complete) {
    return EvalStaged(x, y, config_.debug_stage);
  }

  // Evaluate Uber FBM (includes domain warp, per-octave erosion, etc)
  UberFBMResult uber_result = uber_fbm_->Eval(
      x * config_.frequency, y * config_.frequency, config_.seed, uber_params);

  float value = uber_result.value;
  glm::vec2 gradient = uber_result.derivative;

  // Step 1: Scale by frequency to get dNoise/d(worldSpace)
  glm::vec2 world_gradient = gradient * config_.frequency;

  // Step 2: Scale by amplitude to get dHeight/d(worldSpace)
  world_gradient *= config_.amplitude;

  // Step 3: Compute slope from world-space gradient
  float gradient_magnitude = glm::length(world_gradient);

  // Slope angle in radians (0 = flat, π/2 = vertical)
  float slope_angle = std::atan(gradient_magnitude);

  // Normalize to [0, 1]
  float slope = slope_angle / (3.14159f * 0.5f);

  // Clamp to [0, 1]
  slope = std::clamp(slope, 0.0f, 1.0f);

  // After computing slope, add debug:
  static int debug_count = 0;
  if (debug_count < 5) {
    float slope_degrees = slope * 90.0f;
    std::cout << "DEBUG: noise_deriv=" << glm::length(uber_result.derivative)
              << ", world_grad=" << glm::length(world_gradient)
              << ", slope=" << slope << " (" << slope_degrees << "°)"
              << std::endl;
    debug_count++;
  }

  // Scale by amplitude
  result.height = value * config_.amplitude;
  result.slope = slope;
  result.gradient = gradient;

  // If enabled, apply terracing
  if (config_.terracing.steps > 0 && config_.terracing.smoothness > 0.0f) {
    result.height = Remap::Terrace(result.height, config_.terracing);
  }

  // If enabled, apply plateau
  if (config_.plateau.threshold > 0.0f && config_.plateau.smoothness > 0.0f) {
    result.height = Remap::Plateau(result.height, config_.plateau);
  }

  // Add localised features
  for (size_t fi = 0; fi < config_.features.size(); ++fi) {
    const auto& feature = config_.features[fi];

    float dist = glm::distance(glm::vec2(x, y), feature.center);

    if (dist < feature.radius) {

      int feature_seed = config_.seed + 1000 + static_cast<int>(fi) * 100;

      // Evaluate feature using FastNoise2 (with different seed)
      float feature_value = noise_pipeline_->GenSingle2D(
          (x - feature.center.x) * feature.frequency,
          (y - feature.center.y) * feature.frequency, feature_seed);

      // Apply feature sharpness
      if (feature.sharpness > 0.0f) {
        feature_value = Ridge::Blend(feature_value, feature.sharpness);
      }

      // Scale by feature amplitude
      feature_value *= feature.amplitude;

      // Blend with smooth falloff
      float falloff = ComputeFalloff(dist, feature.radius);
      float blend_amount = falloff * feature.strength;

      // Add to base height
      result.height += feature_value * blend_amount;
    }
  }

  // Build properties for constraint matching
  result.properties.values["height"] = result.height;
  result.properties.values["slope"] = slope;
  result.properties.values["x"] = x;
  result.properties.values["y"] = y;

  // Match rules
  result.matched_rule = config_.constraints.Match(result.properties);

  return result;
}

/* Resources3D::MeshData Generator::GenerateMesh(const MeshOutput& params) const
{ Resources3D::MeshData mesh;

  const uint16_t size = params.size;
  const size_t vcount = size * size;

  mesh.positions_.reserve(vcount * 3);
  if (params.with_normals)
    mesh.normals_.reserve(vcount * 3);
  if (params.with_colors)
    mesh.colors_.reserve(vcount * 4);
  if (params.with_uvs)
    mesh.tex_coords_.reserve(vcount * 2);

  std::vector<float> height_grid(vcount);

  std::vector<glm::vec3> positions_temp(vcount);
  std::vector<glm::vec3> normals_temp(vcount, glm::vec3(0.0f));

  // Generate vertices
  for (uint16_t y = 0; y < size; ++y) {
    for (uint16_t x = 0; x < size; ++x) {
      Sample s = Eval((float)x, (float)y);

      // DEBUG: Print first 20 samples
      static int debug_count = 0;
      if (debug_count < 20) {
        std::cout << "Sample " << debug_count << ": h=" << std::fixed
                  << std::setprecision(2) << s.height << ", s=" << s.slope
                  << ", rule=" << (s.matched_rule ? "YES" : "NO");

        if (s.matched_rule) {
          // Print which rule matched
          auto color_it = s.matched_rule->outputs.find("color");
          if (color_it != s.matched_rule->outputs.end() &&
              color_it->second.size() >= 3) {
            std::cout << ", RGB=[" << color_it->second[0] << ","
                      << color_it->second[1] << "," << color_it->second[2]
                      << "]";
          }
        }
        std::cout << std::endl;
        debug_count++;
      }

      height_grid[y * size + x] = s.height;

      positions_temp[y * size + x] = glm::vec3((float)x, s.height, (float)y);

      // Position
      mesh.positions_.push_back((float)x);  // X = grid column
      mesh.positions_.push_back(s.height);  // Y = height (up axis)
      mesh.positions_.push_back((float)y);  // Z = grid row

      // UV
      if (params.with_uvs) {
        // U = normalized X position
        mesh.tex_coords_.push_back(x / float(size - 1));

        // V = slope (biome blending)
        float normalized_slope = std::clamp(s.slope, 0.0f, 1.0f);
        mesh.tex_coords_.push_back(normalized_slope);
      }

      // Color from rule output
      if (params.with_colors) {
        float h = s.height;
        float slope = s.slope;

        glm::vec3 color;

        if (s.matched_rule) {
          auto it = s.matched_rule->outputs.find("color");
          if (it != s.matched_rule->outputs.end() && it->second.size() >= 3) {
            color = glm::vec3(it->second[0], it->second[1], it->second[2]);
          } else {
            // Fallback: height-based gradient
            float t = glm::clamp((h + 10.0f) / 70.0f, 0.0f, 1.0f);
            color = glm::mix(glm::vec3(0.3f, 0.2f, 0.1f),   // Brown lowlands
                             glm::vec3(0.9f, 0.9f, 0.95f),  // White peaks
                             t);
          }
        } else {
          // No rule matched: simple height gradient
          float t = glm::clamp((h + 10.0f) / 70.0f, 0.0f, 1.0f);
          color = glm::mix(glm::vec3(0.3f, 0.2f, 0.1f),   // Brown lowlands
                           glm::vec3(0.9f, 0.9f, 0.95f),  // White peaks
                           t);
        }

        // Slope-based darkening (shadows)
        float slopeDarken = 1.0f - glm::clamp(slope * 0.12f, 0.0f, 0.25f);
        color *= slopeDarken;

        mesh.colors_.push_back(color.r);
        mesh.colors_.push_back(color.g);
        mesh.colors_.push_back(color.b);

        // Encode height in alpha
        float normalized_height = glm::clamp((h + 50.0f) / 100.0f, 0.0f, 1.0f);
        mesh.colors_.push_back(normalized_height);
      }
    }
  }

  // Generate triangle indices; Expected counter-clockwise
  mesh.indices_.reserve((size - 1) * (size - 1) * 6);
  for (uint16_t y = 0; y < size - 1; ++y) {
    for (uint16_t x = 0; x < size - 1; ++x) {
      uint32_t i = y * size + x;

      // First triangle
      mesh.indices_.push_back(i);
      mesh.indices_.push_back(i + size);
      mesh.indices_.push_back(i + 1);

      // Compute normal for first triangle
      if (params.with_normals) {
        glm::vec3 v0 = positions_temp[i];
        glm::vec3 v1 = positions_temp[i + size];
        glm::vec3 v2 = positions_temp[i + 1];
        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        normals_temp[i] += normal;
        normals_temp[i + size] += normal;
        normals_temp[i + 1] += normal;
      }

      // Second triangle
      mesh.indices_.push_back(i + 1);
      mesh.indices_.push_back(i + size);
      mesh.indices_.push_back(i + size + 1);

      // Compute normal for second triangle
      if (params.with_normals) {
        glm::vec3 v0 = positions_temp[i + 1];
        glm::vec3 v1 = positions_temp[i + size];
        glm::vec3 v2 = positions_temp[i + size + 1];
        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        normals_temp[i + 1] += normal;
        normals_temp[i + size] += normal;
        normals_temp[i + size + 1] += normal;
      }
    }
  }

  // Normalize and output averaged normals
  if (params.with_normals) {
    for (const auto& n : normals_temp) {
      glm::vec3 norm = glm::normalize(n);
      mesh.normals_.push_back(norm.x);
      mesh.normals_.push_back(norm.y);
      mesh.normals_.push_back(norm.z);
    }
  }

  return mesh;
} */

Generator::HeightmapOutput Generator::GenerateHeightmap(uint16_t size) const {
  HeightmapOutput output;
  output.size = size;
  output.heights.resize(size * size);
  output.rgba_encoded.resize(size * size);

  output.min_height = std::numeric_limits<float>::max();
  output.max_height = std::numeric_limits<float>::lowest();

  // Generate heights
  for (uint16_t y = 0; y < size; ++y) {
    for (uint16_t x = 0; x < size; ++x) {
      Sample s = Eval((float)x, (float)y);
      output.heights[y * size + x] = s.height;
      output.min_height = std::min(output.min_height, s.height);
      output.max_height = std::max(output.max_height, s.height);
    }
  }

  // Encode to RGBA
  float range = output.max_height - output.min_height;
  for (uint16_t i = 0; i < size * size; ++i) {
    float normalized = (range > 0.0001f)
                           ? (output.heights[i] - output.min_height) / range
                           : 0.5f;
    normalized = normalized * 2.0f - 1.0f;
    normalized = std::max(-1.0f, std::min(1.0f, normalized));

    uint8_t encoded = static_cast<uint8_t>(127.5f + 127.5f * normalized);
    output.rgba_encoded[i] = (encoded << 24) | (0 << 16) | (0 << 8) | encoded;
  }

  return output;
}

float Generator::ApplyPipeline(float x, float y,
                               glm::vec2& out_gradient) const {
  Sample s = Eval(x, y);
  out_gradient = s.gradient;
  return s.height;
}

Sample Generator::EvalStaged(float x, float y, PipelineStage stage) const {
  Sample result;
  result.curvature = 0.0f;

  float freq_x = x * config_.frequency;
  float freq_y = y * config_.frequency;

  // Build params with selective disabling based on stage
  UberFBMParams uber_params;
  uber_params.octaves = config_.octaves;
  uber_params.lacunarity = config_.lacunarity;
  uber_params.gain = config_.gain;
  uber_params.sharpness = 0.0f;  // Default off
  uber_params.amplify_features = config_.amplify;
  uber_params.altitude_erosion = 0.0f;  // Default off
  uber_params.ridge_erosion = 0.0f;     // Default off
  uber_params.slope_erosion = 0.0f;     // Default off
  uber_params.perturb = 0.0f;           // Default off

  float value = 0.0f;
  glm::vec2 gradient(0.0f);

  switch (stage) {
    case PipelineStage::BaseNoiseOnly: {
      // Raw OpenSimplex2 - single octave, no operators
      double noise_val, dx, dy;
      simplex_deriv_->noise2_deriv(freq_x, freq_y, noise_val, dx, dy);
      value = static_cast<float>(noise_val);
      gradient = glm::vec2(static_cast<float>(dx), static_cast<float>(dy));
      break;
    }

    case PipelineStage::WithDomainWarp: {
      // Domain warp is baked into uber_fbm_ via noise_pipeline_
      // Just disable all other operators to isolate warp effect
      UberFBMResult uber_result =
          uber_fbm_->Eval(freq_x, freq_y, config_.seed, uber_params);
      value = uber_result.value;
      gradient = uber_result.derivative;
      break;
    }

    case PipelineStage::WithSharpness: {
      // Enable sharpness only
      uber_params.sharpness = config_.sharpness;
      UberFBMResult uber_result =
          uber_fbm_->Eval(freq_x, freq_y, config_.seed, uber_params);
      value = uber_result.value;
      gradient = uber_result.derivative;
      break;
    }

    case PipelineStage::WithSlopeErosion: {
      // Enable sharpness + slope erosion
      uber_params.sharpness = config_.sharpness;
      uber_params.slope_erosion = config_.slope_erosion;
      UberFBMResult uber_result =
          uber_fbm_->Eval(freq_x, freq_y, config_.seed, uber_params);
      value = uber_result.value;
      gradient = uber_result.derivative;
      break;
    }

    case PipelineStage::WithRidgeErosion: {
      // Enable sharpness + slope + ridge erosion
      uber_params.sharpness = config_.sharpness;
      uber_params.slope_erosion = config_.slope_erosion;
      uber_params.ridge_erosion = config_.ridge_erosion;
      UberFBMResult uber_result =
          uber_fbm_->Eval(freq_x, freq_y, config_.seed, uber_params);
      value = uber_result.value;
      gradient = uber_result.derivative;
      break;
    }

    case PipelineStage::WithAltitudeErosion: {
      // Enable sharpness + all erosion
      uber_params.sharpness = config_.sharpness;
      uber_params.slope_erosion = config_.slope_erosion;
      uber_params.ridge_erosion = config_.ridge_erosion;
      uber_params.altitude_erosion = config_.altitude_erosion;
      UberFBMResult uber_result =
          uber_fbm_->Eval(freq_x, freq_y, config_.seed, uber_params);
      value = uber_result.value;
      gradient = uber_result.derivative;
      break;
    }

    case PipelineStage::Complete:
    default: {
      // Full pipeline - call normal Eval
      return Eval(x, y);
    }
  }

  // Compute slope from gradient
  glm::vec2 world_gradient = gradient * config_.frequency * config_.amplitude;
  float gradient_magnitude = glm::length(world_gradient);
  float slope_angle = std::atan(gradient_magnitude);
  float slope = std::clamp(slope_angle / (3.14159f * 0.5f), 0.0f, 1.0f);

  // Fill result
  result.height = value * config_.amplitude;
  result.slope = slope;
  result.gradient = gradient;

  // Build properties for constraint matching
  result.properties.values["height"] = result.height;
  result.properties.values["slope"] = slope;
  result.properties.values["x"] = x;
  result.properties.values["y"] = y;

  result.matched_rule = config_.constraints.Match(result.properties);

  return result;
}

}  // namespace PCG