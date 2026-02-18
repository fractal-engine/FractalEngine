#ifndef GEOMETRY_DATA_H
#define GEOMETRY_DATA_H

#include <cstdint>
#include <vector>

namespace Geometry {

// ! Rename file to mesh_data.h
// ! move file to engine/geometry

// Vertex - index data
struct MeshData {
  std::vector<float> positions;   // xyz
  std::vector<float> normals;     // xyz,
  std::vector<float> colors;      // rgba
  std::vector<float> tex_coords;  // uv
  std::vector<uint32_t> indices;

  size_t VertexCount() const { return positions.size() / 3; }
  size_t TriangleCount() const { return indices.size() / 3; }
  bool HasNormals() const { return !normals.empty(); }
  bool HasColors() const { return !colors.empty(); }
  bool HasUVs() const { return !tex_coords.empty(); }

  void Clear() {
    positions.clear();
    normals.clear();
    colors.clear();
    tex_coords.clear();
    indices.clear();
  }

  void Reserve(size_t vertex_count, size_t index_count) {
    positions.reserve(vertex_count * 3);
    normals.reserve(vertex_count * 3);
    colors.reserve(vertex_count * 4);
    tex_coords.reserve(vertex_count * 2);
    indices.reserve(index_count);
  }
};

}  // namespace Geometry

#endif  // GEOMETRY_DATA_H