#ifndef TRANSFORM_SYSTEM_H
#define TRANSFORM_SYSTEM_H

#include <glm/glm.hpp>
#include <vector>

#include "transform.h"

class TransformSystem {
public:
  // Perform preprocessing before rendering all entities
  void Perform(glm::mat4 viewProjection);

private:
  // Update model matrix (local or with parent)
  void Evaluate(TransformComponent& transform);
  void Evaluate(TransformComponent& transform, const TransformComponent& parent,
                bool propagateModified);
};

#endif  // TRANSFORM_SYSTEM_H