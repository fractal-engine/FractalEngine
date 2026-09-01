#define STB_PERLIN_IMPLEMENTATION
#include "engine/math/noise/stb_perlin.h"

#include "perlin.h"

namespace Math {

float Perlin3D(float x, float y, float z) {
  return stb_perlin_noise3(x, y, z, 0, 0, 0);
}

}  // namespace Math