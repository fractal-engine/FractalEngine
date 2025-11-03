#include "simplex.h"

#include <cmath>
#include <utility>

namespace Generator {

// Gradient vectors
static const int grad3[12][3] = {
    {1, 1, 0},  {-1, 1, 0},  {1, -1, 0}, {-1, -1, 0}, {1, 0, 1},  {-1, 0, 1},
    {1, 0, -1}, {-1, 0, -1}, {0, 1, 1},  {0, -1, 1},  {0, 1, -1}, {0, -1, -1}};

SimplexNoise::SimplexNoise(uint32_t seed) {
  // Init permutation table + seed
  uint8_t p[256];
  for (int i = 0; i < 256; ++i)
    p[i] = i;

  // Fisher-Yates shuffle + seed
  uint32_t rng = seed;
  for (int i = 255; i > 0; --i) {
    rng = rng * 1664525u + 1013904223u;
    int j = rng % (i + 1);
    std::swap(p[i], p[j]);
  }

  for (int i = 0; i < 512; ++i) {
    perm_[i] = p[i & 255];
  }
}

int SimplexNoise::FastFloor(float x) {
  return x > 0 ? (int)x : (int)x - 1;
}

float SimplexNoise::Dot(const int* g, float x, float y) {
  return g[0] * x + g[1] * y;
}

float SimplexNoise::Noise2D(float x, float y) const {
  const float F2 = 0.5f * (sqrtf(3.0f) - 1.0f);
  const float G2 = (3.0f - sqrtf(3.0f)) / 6.0f;

  float s = (x + y) * F2;
  int i = FastFloor(x + s);
  int j = FastFloor(y + s);

  float t = (i + j) * G2;
  float X0 = i - t;
  float Y0 = j - t;
  float x0 = x - X0;
  float y0 = y - Y0;

  int i1 = (x0 > y0) ? 1 : 0;
  int j1 = (x0 > y0) ? 0 : 1;

  float x1 = x0 - i1 + G2;
  float y1 = y0 - j1 + G2;
  float x2 = x0 - 1.0f + 2.0f * G2;
  float y2 = y0 - 1.0f + 2.0f * G2;

  int ii = i & 255;
  int jj = j & 255;
  int gi0 = perm_[ii + perm_[jj]] % 12;
  int gi1 = perm_[ii + i1 + perm_[jj + j1]] % 12;
  int gi2 = perm_[ii + 1 + perm_[jj + 1]] % 12;

  float n0 = 0.0f, n1 = 0.0f, n2 = 0.0f;
  float t0 = 0.5f - x0 * x0 - y0 * y0;
  if (t0 >= 0) {
    t0 *= t0;
    n0 = t0 * t0 * Dot(grad3[gi0], x0, y0);
  }

  float t1 = 0.5f - x1 * x1 - y1 * y1;
  if (t1 >= 0) {
    t1 *= t1;
    n1 = t1 * t1 * Dot(grad3[gi1], x1, y1);
  }

  float t2 = 0.5f - x2 * x2 - y2 * y2;
  if (t2 >= 0) {
    t2 *= t2;
    n2 = t2 * t2 * Dot(grad3[gi2], x2, y2);
  }

  return 70.0f * (n0 + n1 + n2);
}

SimplexResult SimplexNoise::NoiseWithDerivatives(float x, float y) const {
  // Detailed derivative calculation (Quilez method)
  const float F2 = 0.5f * (sqrtf(3.0f) - 1.0f);
  const float G2 = (3.0f - sqrtf(3.0f)) / 6.0f;

  SimplexResult result{0.0f, glm::vec2(0.0f)};

  // TODO: simplex calculation with derivative accumulation
  result.value = Noise2D(x, y);

  // Numerical derivatives for now
  // TODO: replace with analytical later
  const float h = 0.01f;
  result.derivative.x = (Noise2D(x + h, y) - Noise2D(x - h, y)) / (2.0f * h);
  result.derivative.y = (Noise2D(x, y + h) - Noise2D(x, y - h)) / (2.0f * h);

  return result;
}

}  // namespace Generator