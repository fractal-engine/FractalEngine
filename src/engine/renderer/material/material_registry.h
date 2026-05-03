#ifndef MATERIAL_REGISTRY_H
#define MATERIAL_REGISTRY_H

#include <cstdint>
#include <unordered_map>

#include "engine/core/types/material_data.h"

namespace Renderer {

using MaterialHandle = uint32_t;
constexpr MaterialHandle INVALID_MATERIAL = 0;

class MaterialRegistry {
public:
  static MaterialRegistry& Instance();

  // Register a material and return its handle
  MaterialHandle Register(const Content::MaterialData& data);

  // Get material data by handle (returns nullptr if invalid)
  const Content::MaterialData* Get(MaterialHandle handle) const;

  // Unregister a material by handle
  void Unregister(MaterialHandle handle);

  // Clear all materials
  void Clear();

private:
  MaterialRegistry() = default;

  std::unordered_map<MaterialHandle, Content::MaterialData> materials_;
  MaterialHandle next_handle_{1};
};

}  // namespace Renderer

#endif  // MATERIAL_REGISTRY_H