#ifndef SAMPLE_H
#define SAMPLE_H

#include <glm/glm.hpp>

#include "../constraints/constraint_system.h"  // ! remove dependency

namespace PCG {

//=============================================================================
// SAMPLE RESULT - What generators produce at a point
//=============================================================================
struct Sample {
  float height = 0.0f;
  float slope = 0.0f;
  float curvature = 0.0f;
  glm::vec2 gradient{0.0f};
  Properties properties;
  const Rule* matched_rule = nullptr;
};

}  // namespace PCG

#endif  // SAMPLE_H