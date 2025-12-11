#ifndef VOLUME_COMPONENT_H
#define VOLUME_COMPONENT_H

#include <cstdint>
#include <glm/glm.hpp>

// Resource IDs
using ResourceID = uint32_t;

struct VolumeComponent {
  // ─────────── Resource References ───────────
  ResourceID generator_id = 0;  // Generator resource
  ResourceID stream_id = 0;     // Persistent data source (stream)
  ResourceID mesher_id = 0;     // Mesh generation strategy (mesher)

  // ─────────── Volume Bounds ───────────
  glm::vec3 bounds_min{0.0f};
  glm::vec3 bounds_max{512.0f};

  // ─────────── Generation Settings ───────────
  uint16_t resolution = 512;  // Grid resolution
  uint8_t lod_level = 0;      // Level of detail

  // ─────────── State ───────────
  bool dirty = true;       // Needs regeneration
  bool streaming = false;  // Uses streaming
};

#endif  // VOLUME_COMPONENT_H