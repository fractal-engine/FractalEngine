#include "mesh.h"

#include <cstring>
#include "engine/core/logger.h"

bgfx::VertexLayout Mesh::layout_;

void Mesh::EnsureLayout() {
  if (layout_.getStride() != 0)
    return;

  // ? Color0 has been removed
  layout_.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Tangent, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Bitangent, 3, bgfx::AttribType::Float)
      .end();
}

Mesh::Mesh(const Geometry::MeshData& src) {
  EnsureLayout();

  static_assert(
      sizeof(Geometry::VertexData) == (3 + 3 + 2 + 3 + 3) * sizeof(float),
      "VertexData has unexpected padding");

  // VertexData is already interleaved — pass directly to bgfx
  const bgfx::Memory* vmem = bgfx::copy(
      src.vertices.data(), sizeof(Geometry::VertexData) * src.vertices.size());
  vbo_ = bgfx::createVertexBuffer(vmem, layout_);

  const bgfx::Memory* imem =
      bgfx::copy(src.indices.data(), sizeof(uint32_t) * src.indices.size());
  ibo_ = bgfx::createIndexBuffer(imem, BGFX_BUFFER_INDEX32);

  index_count_ = static_cast<uint32_t>(src.indices.size());
}

Mesh::~Mesh() {
  if (bgfx::isValid(vbo_))
    bgfx::destroy(vbo_);
  if (bgfx::isValid(ibo_))
    bgfx::destroy(ibo_);
}

void Mesh::Bind() const {
  bgfx::setVertexBuffer(0, vbo_);
  bgfx::setIndexBuffer(ibo_);
}
