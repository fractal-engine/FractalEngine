#include "mesh.h"

#include <cstring>
#include "engine/core/logger.h"

bgfx::VertexLayout Mesh::layout_;

void Mesh::EnsureLayout() {
  if (layout_.getStride() != 0)
    return;

  // TODO: check if we should have Color0 set here
  layout_.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float)
      .end();
}

Mesh::Mesh(const Geometry::MeshData& src) {
  EnsureLayout();

  // interleave pos+normal (if no normals -> pad zeros)
  const bool has_normals = src.HasNormals();
  const bool has_colors = src.HasColors();
  const size_t v_count = src.VertexCount();

  // 3 (pos) + 3 (normal) + 4 (color)
  std::vector<float> interleaved;
  interleaved.reserve(v_count * 10);

  for (size_t i = 0; i < v_count; ++i) {
    // Position
    interleaved.push_back(src.positions[i * 3 + 0]);
    interleaved.push_back(src.positions[i * 3 + 1]);
    interleaved.push_back(src.positions[i * 3 + 2]);

    // Normal
    if (has_normals) {
      interleaved.push_back(src.normals[i * 3 + 0]);
      interleaved.push_back(src.normals[i * 3 + 1]);
      interleaved.push_back(src.normals[i * 3 + 2]);
    } else {
      interleaved.push_back(0.f);
      interleaved.push_back(1.f);
      interleaved.push_back(0.f);
    }

    // Color
    if (has_colors) {
      interleaved.push_back(src.colors[i * 4 + 0]);
      interleaved.push_back(src.colors[i * 4 + 1]);
      interleaved.push_back(src.colors[i * 4 + 2]);
      interleaved.push_back(src.colors[i * 4 + 3]);
    } else {
      interleaved.push_back(1.f);
      interleaved.push_back(1.f);
      interleaved.push_back(1.f);
      interleaved.push_back(1.f);
    }
  }

  const bgfx::Memory* vmem =
      bgfx::copy(interleaved.data(), sizeof(float) * interleaved.size());
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
