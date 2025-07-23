#ifndef MESH_RENDERER_H
#define MESH_RENDERER_H

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "engine/renderer/model/mesh.h"

/* ────────────────────────────── MESH RENDERER ──────────────────────────── */
struct MeshRendererComponent {
  const Mesh* mesh_{nullptr};
  bool enabled_{true};
};

#endif  // MESH_RENDERER_H