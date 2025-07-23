#ifndef MESH_H
#define MESH_H

#include <bgfx/bgfx.h>
#include <vector>

#include "engine/formats/gltf_translator.h"
#include "engine/resources/3d/mesh_data.h"

class Mesh {
public:
  explicit Mesh(const Resources3D::MeshData& src);
  ~Mesh();

  // bind VBO/IBO
  void Bind() const;

  uint32_t IndexCount() const { return index_count_; }

private:
  bgfx::VertexBufferHandle vbo_ = BGFX_INVALID_HANDLE;
  bgfx::IndexBufferHandle ibo_ = BGFX_INVALID_HANDLE;
  uint32_t index_count_{0};

  static bgfx::VertexLayout layout_;
  static void EnsureLayout();
};

#endif  // MESH_H
