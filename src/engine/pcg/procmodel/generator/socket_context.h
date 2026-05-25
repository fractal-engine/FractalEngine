#ifndef SOCKET_CONTEXT_H
#define SOCKET_CONTEXT_H

#include <glm/glm.hpp>
#include <pcg_random.hpp>
#include <string>

#include "engine/pcg/pipeline/operation_context.h"

namespace ProcModel {

// Context passed to socket modifiers.
// Modifiers receive the resolved socket world transform and may mutate it
// before the part is placed. activator_world is the world transform of the
// part this socket sits on, used by modifiers like radial_align to derive
// outward directions.
struct SocketContext : public PCG::OperationContext {
  glm::mat4& socket_world;           // mutable: modifier writes here
  const glm::mat4& activator_world;  // read-only: parent's world transform
  const std::string& socket_id;      // read-only: for logging/debug
  pcg32& rng;

  SocketContext(glm::mat4& sw, const glm::mat4& aw, const std::string& sid,
                pcg32& r)
      : socket_world(sw), activator_world(aw), socket_id(sid), rng(r) {}
};

}  // namespace ProcModel

#endif  // SOCKET_CONTEXT_H