#include "ridge.h"
#include <cmath>

namespace PCG {

float Ridge::Blend(float value, float sharpness) {
  float billow = std::abs(value);
  float ridged = 1.0f - std::abs(value);

  if (sharpness < 0.0f) {
    // Blend between ridge and original
    return ridged * (-sharpness) + value * (1.0f + sharpness);
  } else {
    // Blend between original and billow
    return value * (1.0f - sharpness) + billow * sharpness;
  }
}

}  // namespace PCG