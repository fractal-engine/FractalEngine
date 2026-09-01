#ifndef LOCATOR_CONTEXT_H
#define LOCATOR_CONTEXT_H

#include <glm/glm.hpp>
#include <pcg_random.hpp>
#include <string>

#include "engine/pcg/pipeline/operation_context.h"
#include "engine/pcg/procmodel/validation/parameter_sampler.h"

namespace ProcModel {

// Context passed to locator modifiers.
// Modifiers receive the resolved locator world transform and may mutate it
// before the part is placed. activator_world is the world transform of the
// part this locator sits on, used by modifiers like radial_align to derive
// outward directions.
struct LocatorContext : public PCG::OperationContext {
  glm::mat4& locator_world;          // mutable: modifier writes here
  const glm::mat4& activator_world;  // read-only: parent's world transform
  const std::string& locator_id;     // read-only: for logging/debug
  pcg32& rng;
  ParameterSampler& sampler;

  LocatorContext(glm::mat4& lw, const glm::mat4& aw, const std::string& lid,
                 pcg32& r, ParameterSampler& s)
      : locator_world(lw),
        activator_world(aw),
        locator_id(lid),
        rng(r),
        sampler(s) {}
};

}  // namespace ProcModel

#endif  // LOCATOR_CONTEXT_H