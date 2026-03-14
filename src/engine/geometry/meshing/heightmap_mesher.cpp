#include "heightmap_mesher.h"

#include <glm/glm.hpp>

namespace Geometry {

HeightmapMesher::HeightmapMesher(HeightSampleFn sample_fn,
                                 const MesherParams& params)
    : sample_fn_(std::move(sample_fn)), params_(params) {}

MeshData HeightmapMesher::Generate() const {
  MeshData mesh;

  const uint16_t size = params_.size;
  const size_t vcount = static_cast<size_t>(size) * size;

  mesh.Reserve(vcount, (size - 1) * (size - 1) * 6);

  std::vector<glm::vec3> positions_temp(vcount);
  std::vector<glm::vec3> normals_temp(vcount, glm::vec3(0.0f));

  // Generate vertices
  for (uint16_t y = 0; y < size; ++y) {
    for (uint16_t x = 0; x < size; ++x) {
      HeightSample s = sample_fn_(static_cast<float>(x), static_cast<float>(y));

      size_t idx = y * size + x;
      positions_temp[idx] =
          glm::vec3(static_cast<float>(x), s.height, static_cast<float>(y));

      // Position
      mesh.positions.push_back(static_cast<float>(x));
      mesh.positions.push_back(s.height);
      mesh.positions.push_back(static_cast<float>(y));

      // UV
      if (params_.with_uvs) {
        mesh.tex_coords.push_back(x / static_cast<float>(size - 1));
        mesh.tex_coords.push_back(s.slope);  // V = slope for biome blending
      }

      // Color
      if (params_.with_colors) {
        mesh.colors.push_back(s.color.r);
        mesh.colors.push_back(s.color.g);
        mesh.colors.push_back(s.color.b);

        // Encode height in alpha
        float normalized_height =
            glm::clamp((s.height + 50.0f) / 100.0f, 0.0f, 1.0f);
        mesh.colors.push_back(normalized_height);
      }
    }
  }

  // Generate indices and compute normals
  for (uint16_t y = 0; y < size - 1; ++y) {
    for (uint16_t x = 0; x < size - 1; ++x) {
      uint32_t i = y * size + x;

      // First triangle
      mesh.indices.push_back(i);
      mesh.indices.push_back(i + size);
      mesh.indices.push_back(i + 1);

      if (params_.with_normals) {
        glm::vec3 v0 = positions_temp[i];
        glm::vec3 v1 = positions_temp[i + size];
        glm::vec3 v2 = positions_temp[i + 1];
        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        normals_temp[i] += normal;
        normals_temp[i + size] += normal;
        normals_temp[i + 1] += normal;
      }

      // Second triangle
      mesh.indices.push_back(i + 1);
      mesh.indices.push_back(i + size);
      mesh.indices.push_back(i + size + 1);

      if (params_.with_normals) {
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
  if (params_.with_normals) {
    for (const auto& n : normals_temp) {
      glm::vec3 norm = glm::normalize(n);
      mesh.normals.push_back(norm.x);
      mesh.normals.push_back(norm.y);
      mesh.normals.push_back(norm.z);
    }
  }

  return mesh;
}

}  // namespace Geometry