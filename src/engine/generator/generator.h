#ifndef GENERATOR_H
#define GENERATOR_H

#include <FastNoise/FastNoise.h>

#include "constraints/constraint_system.h"
#include "feature_descriptors.h"
#include "operators/fbm.h"
#include "operators/remap.h"
#include "operators/ridge.h"

// Forward declare MeshData
namespace Resources3D {
struct MeshData;
}

namespace Generator {

struct Config {
  uint32_t seed = 12347;
  float frequency = 0.01f;
  float amplitude = 100.0f;

  // Operator params
  int octaves = 6;                // liOctaves
  float perturb = 0.5f;           // lfPerturbFeatures
  float sharpness = 0.0f;         // lfSharpness
  float amplify = 1.0f;           // lfAmplifyFeatures
  float altitude_erosion = 0.0f;  // lfAltitudeErosion
  float ridge_erosion = 0.0f;     // lfRidgeErosion
  float slope_erosion = 0.0f;     // lfSlopeErosion
  float lacunarity = 2.0f;        // lfLacunarity
  float gain = 0.5f;              // lfGain

  // Parameter variation
  struct ParameterVariation {
    float frequency = 0.01f;  // Spatial frequency of variation
    float amplitude = 0.3f;   // How much it varies
    bool enabled = false;
  };

  ParameterVariation vary_sharpness;
  ParameterVariation vary_perturb;
  ParameterVariation vary_amplify;

  // Constraint system
  ConstraintSystem constraints;
  std::vector<Feature> features;

  // Remap
  TerracingParams terracing = {5, 0.0f};
  PlateauParams plateau = {0.6f, 0.1f};
};

// Single point sample result
struct Sample {
  float height;
  float slope;
  float curvature;
  glm::vec2 gradient;
  Properties properties;
  const Rule* matched_rule = nullptr;
};

class Generator {
public:
  Generator(const Config& config);

  // Generate single sample
  Sample Eval(float x, float y) const;

  // Generate heightmap data
  struct HeightmapOutput {
    std::vector<float> heights;
    std::vector<uint32_t> rgba_encoded;
    uint16_t size;
    float min_height;
    float max_height;
  };

  HeightmapOutput GenerateHeightmap(uint16_t size) const;

  // Generate mesh data
  struct MeshOutput {
    uint16_t size;
    bool with_normals = true;
    bool with_colors = true;
    bool with_uvs = true;
  };
  Resources3D::MeshData GenerateMesh(const MeshOutput& params) const;

  void ExportNodeGraph(const std::string& filename) const;

  void UpdateConfig(const Config& config);
  const Config& GetConfig() const { return config_; }

private:
  Config config_;

  FastNoise::SmartNode<> noise_pipeline_;
  std::unique_ptr<UberFBM> uber_fbm_;

  float ApplyPipeline(float x, float y, glm::vec2& out_gradient) const;
};
}  // namespace Generator

#endif  // GENERATOR_H