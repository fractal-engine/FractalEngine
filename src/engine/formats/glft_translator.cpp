#include "gltf_translator.h"

#include <tiny_gltf.h>
#include "engine/core/logger.h"
#include "engine/resources/3d/mesh_data.h"

using namespace tinygltf;

namespace Formats {

static std::vector<uint8_t> ReadAccessorData(const Model& model,
                                             const Accessor& accessor) {
  const auto& view = model.bufferViews[accessor.bufferView];
  const auto& buffer = model.buffers[view.buffer];
  const uint8_t* data = buffer.data.data() + view.byteOffset + accessor.byteOffset;
  return {data, data + view.byteLength};
}

std::vector<Resources3D::MeshData> TranslateGLTF(const std::string& path) {
  Model model;
  TinyGLTF loader;
  std::string err, warn;

  const bool ok = path.size() > 4 && path.substr(path.size() - 4) == ".glb"
                      ? loader.LoadBinaryFromFile(&model, &err, &warn, path)
                      : loader.LoadASCIIFromFile(&model, &err, &warn, path);

  if (!warn.empty())
    Logger::getInstance().Log(LogLevel::Warning, warn);
  if (!err.empty())
    Logger::getInstance().Log(LogLevel::Error, err);
  if (!ok)
    return {};

  std::vector<Resources3D::MeshData> result;

  for (const Mesh& mesh : model.meshes) {
    for (const Primitive& prim : mesh.primitives) {
      Resources3D::MeshData out;

      // positions
      const Accessor& pos_accc =
          model.accessors.at(prim.attributes.at("POSITION"));
      auto pos_raw = ReadAccessorData(model, pos_accc);
      out.positions_.resize(pos_accc.count * 3);
      memcpy(out.positions_.data(), pos_raw.data(), pos_raw.size());

      // normals
      auto n_it = prim.attributes.find("NORMAL");
      if (n_it != prim.attributes.end()) {
        const Accessor& n_acc = model.accessors.at(n_it->second);
        auto n_raw = ReadAccessorData(model, n_acc);
        out.normals_.resize(n_acc.count * 3);
        memcpy(out.normals_.data(), n_raw.data(), n_raw.size());
      }

      // indices
      const Accessor& idx_acc = model.accessors[prim.indices];
      auto idx_raw = ReadAccessorData(model, idx_acc);
      out.indices_.resize(idx_acc.count);
      const void* src = idx_raw.data();

      if (idx_acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        const uint16_t* s = static_cast<const uint16_t*>(src);
        for (size_t i = 0; i < idx_acc.count; ++i)
          out.indices_[i] = s[i];
      } else {  // already u32
        memcpy(out.indices_.data(), src, idx_raw.size());
      }

      // material
      // TODO: check if this needs to use Material implementation instead
      if (prim.material >= 0 && prim.material < (int)model.materials.size())
        out.material_name_ = model.materials[prim.material].name;

      result.emplace_back(std::move(out));
    }
  }
  return result;
}

}  // namespace Formats
