#ifndef ROTATION_JITTER_H
#define ROTATION_JITTER_H

#include <glm/vec3.hpp>

#include "engine/pcg/pipeline/operation_data.h"
#include "engine/pcg/pipeline/operation_registry.h"

namespace ProcModel {

// TODO: rename to RotationJitterData and RotationJitter
// TODO: use Transform operations and Transformation system instead of glm

// Per-socket rotation perturbation. For each socket, draws a uniform
// random angle from [-yaw, +yaw], [-pitch, +pitch], [-roll, +roll] and
// composes it into the socket's frame. Same authored range produces
// different perturbations per socket via independent RNG draws.
struct SocketRotationJitterData : public PCG::OperationData {
  // Stored in radians; parser converts from degrees.
  glm::vec3 range = glm::vec3(0.0f);  // x = pitch, y = yaw, z = roll
};

void RegisterSocketRotationJitterOperation(PCG::OperationRegistry& registry);

}  // namespace ProcModel

#endif  // ROTATION_JITTER_H