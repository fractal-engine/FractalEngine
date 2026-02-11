#ifndef MARCHING_CUBES_MESHER_H
#define MARCHING_CUBES_MESHER_H

#include <functional>
#include <glm/glm.hpp>

#include "engine/core/types/geometry_data.h"

namespace Geometry {

// Return  signed distance at position (negative = inside)
using SDFSampleFn = std::function<float(const glm::vec3& pos)>;

// Vertex attributes at position
struct VolumeSample {
  float sdf;         // Signed distance
  glm::vec3 color;   // vertex color
  glm::vec3 normal;  // analytic normal
};
using VolumeSampleFn = std::function<VolumeSample(const glm::vec3& pos)>;

//=============================================================================
// MARCHING CUBES
//=============================================================================
struct MarchingCubesParams {
  glm::vec3 bounds_min{0.0f};
  glm::vec3 bounds_max{1.0f};
  glm::uvec3 resolution{32};  // Cells per axis
  float iso_level = 0.0f;     // Surface threshold
  bool compute_normals = true;
  bool with_colors = false;
};

class MarchingCubesMesher {
public:
  explicit MarchingCubesMesher(SDFSampleFn sample_fn,
                               const MarchingCubesParams& params);

  // full vertex attributes
  MarchingCubesMesher(VolumeSampleFn sample_fn,
                      const MarchingCubesParams& params);

  MeshData Generate() const;

private:
  SDFSampleFn sdf_fn_;
  VolumeSampleFn volume_fn_;
  MarchingCubesParams params_;
  bool use_volume_sampler_ = false;

  void ProcessCell(const glm::uvec3& cell, MeshData& out) const;
  glm::vec3 InterpolateEdge(const glm::vec3& p1, const glm::vec3& p2, float v1,
                            float v2) const;
};

//=============================================================================
// TRANSVOXEL - LOD extension
//=============================================================================
struct TransvoxelParams : MarchingCubesParams {
  uint8_t lod_level = 0;
  // Neighbor LOD levels (transition cells)
  // 0=same, positive=coarser neighbor
  uint8_t neighbor_lod[6] = {0, 0, 0, 0, 0, 0};  // -X,+X,-Y,+Y,-Z,+Z
};

class TransvoxelMesher {
public:
  TransvoxelMesher(SDFSampleFn sample_fn, const TransvoxelParams& params);

  MeshData Generate() const;

private:
  SDFSampleFn sdf_fn_;
  TransvoxelParams params_;

  void ProcessRegularCell(const glm::uvec3& cell, MeshData& out) const;
  void ProcessTransitionCell(const glm::uvec3& cell, uint8_t face,
                             MeshData& out) const;
};

}  // namespace Geometry

#endif  // MARCHING_CUBES_MESHER_H