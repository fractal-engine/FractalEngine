#include "material_registry.h"

#include "engine/core/logger.h"

namespace Renderer {

MaterialRegistry& MaterialRegistry::Instance() {
  static MaterialRegistry instance;
  return instance;
}

MaterialHandle MaterialRegistry::Register(const Content::MaterialData& data) {
  MaterialHandle handle = next_handle_++;
  materials_[handle] = data;

  Logger::getInstance().Log(
      LogLevel::Debug, "[MaterialRegistry] Registered material '" + data.name +
                           "' with handle " + std::to_string(handle));

  return handle;
}

const Content::MaterialData* MaterialRegistry::Get(
    MaterialHandle handle) const {
  if (handle == INVALID_MATERIAL) {
    return nullptr;
  }

  auto it = materials_.find(handle);
  if (it != materials_.end()) {
    return &it->second;
  }

  return nullptr;
}

void MaterialRegistry::Unregister(MaterialHandle handle) {
  auto it = materials_.find(handle);
  if (it != materials_.end()) {
    Logger::getInstance().Log(LogLevel::Debug,
                              "[MaterialRegistry] Unregistered material '" +
                                  it->second.name + "' (handle " +
                                  std::to_string(handle) + ")");
    materials_.erase(it);
  }
}

void MaterialRegistry::Clear() {
  materials_.clear();
  next_handle_ = 1;
  Logger::getInstance().Log(LogLevel::Debug,
                            "[MaterialRegistry] Cleared all materials");
}

}  // namespace Renderer