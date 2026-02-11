#ifndef TERRAIN_GENERATOR_H
#define TERRAIN_GENERATOR_H

#include <iomanip>
#include <memory>

#include "../constraints/constraint_system.h"
#include "../core/feature_descriptors.h"
#include "../core/sample.h"
#include "../operators/fbm.h"
#include "../operators/remap.h"
#include "../operators/ridge.h"

// Forward declare MeshData
/* namespace Geometry {
struct MeshData;
} */

namespace PCG {

enum class PipelineStage {
  BaseNoiseOnly,        // Raw OpenSimplex2
  WithDomainWarp,       // + FastNoise2 domain warp
  WithSharpness,        // + Ridge/Billow blending
  WithSlopeErosion,     // + Slope erosion
  WithRidgeErosion,     // + Ridge erosion
  WithAltitudeErosion,  // + Altitude erosion
  Complete              // Final output
};

struct Config {
  uint32_t seed = 12347;
  float frequency = 0.05f;
  float amplitude = 60.0f;

  // Operator params
  int octaves = 6;                // liOctaves
  float perturb = 0.7f;           // lfPerturbFeatures
  float sharpness = -0.5f;        // lfSharpness
  float amplify = 1.0f;           // lfAmplifyFeatures
  float altitude_erosion = 0.2f;  // lfAltitudeErosion
  float ridge_erosion = 0.6f;     // lfRidgeErosion
  float slope_erosion = 0.4f;     // lfSlopeErosion
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

  // Debug
  PipelineStage debug_stage = PipelineStage::Complete;
};

class Generator {
public:
  Generator(const Config& config);

  // Generate single sample
  Sample Eval(float x, float y) const;

  // Debug: stage evaluation
  Sample EvalStaged(float x, float y, PipelineStage stage) const;

  // ! commented out to be removed, now replaced by geometry/meshing
  // Generate heightmap data
  /* struct HeightmapOutput {
    std::vector<float> heights;
    std::vector<uint32_t> rgba_encoded;
    uint16_t size;
    float min_height;
    float max_height;
  }; */

  // HeightmapOutput GenerateHeightmap(uint16_t size) const;

  // Generate mesh data
  /* struct MeshOutput {
    uint16_t size;
    bool with_normals = true;
    bool with_colors = true;
    bool with_uvs = true;
  };*/

  // Geometry::MeshData GenerateMesh(const MeshOutput& params) const;

  void ExportNodeGraph(const std::string& filename) const;

  void UpdateConfig(const Config& config);
  const Config& GetConfig() const { return config_; }

private:
  Config config_;

  FastNoise::SmartNode<> noise_pipeline_;
  std::unique_ptr<UberFBM> uber_fbm_;

  std::unique_ptr<OpenSimplex2S> simplex_deriv_;

  float ApplyPipeline(float x, float y, glm::vec2& out_gradient) const;
};
}  // namespace PCG

#endif  // TERRAIN_GENERATOR_H