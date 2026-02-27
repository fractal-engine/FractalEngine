#ifndef MODEL_H
#define MODEL_H

#include <bgfx/bgfx.h>
#include <memory>
#include <vector>
#include <string>

#include "engine/renderer/model/mesh.h"

class Model {
public:
  static std::shared_ptr<Model> Load(const std::string& file);

  void Draw(bgfx::ViewId view, bgfx::ProgramHandle program) const;

  uint32_t NLoadedMeshes() const;

  const Mesh* QueryMesh(uint32_t index) const;

private:
  std::vector<std::unique_ptr<Mesh>> meshes_;
};

#endif  // MODEL_H
