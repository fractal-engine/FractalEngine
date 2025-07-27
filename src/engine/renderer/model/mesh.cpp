#include "mesh.h"

#include <cstring>
#include "engine/core/logger.h"

bgfx::VertexLayout Mesh::layout_;

void Mesh::EnsureLayout() {
  if (layout_.getStride() != 0)
    return;

  layout_.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
      .end();
}

Mesh::Mesh(const Resources3D::MeshData& src) {
  EnsureLayout();

  // interleave pos+normal (if no normals -> pad zeros)
  const bool has_normals = !src.normals_.empty();
  const size_t v_count = src.positions_.size() / 3;
  std::vector<float> interleaved;
  interleaved.reserve(v_count * 6);
  for (size_t i = 0; i < v_count; ++i) {
    interleaved.insert(interleaved.end(), &src.positions_[i * 3],
                       &src.positions_[i * 3] + 3);
    if (has_normals)
      interleaved.insert(interleaved.end(), &src.normals_[i * 3],
                         &src.normals_[i * 3] + 3);
    else
      interleaved.insert(interleaved.end(), {0.f, 0.f, 1.f});
  }

  const bgfx::Memory* vmem =
      bgfx::copy(interleaved.data(), sizeof(float) * interleaved.size());
  vbo_ = bgfx::createVertexBuffer(vmem, layout_);

  const bgfx::Memory* imem =
      bgfx::copy(src.indices_.data(), sizeof(uint32_t) * src.indices_.size());
  ibo_ = bgfx::createIndexBuffer(imem, BGFX_BUFFER_INDEX32);

  index_count_ = static_cast<uint32_t>(src.indices_.size());
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
