#ifndef PERLIN_H
#define PERLIN_H

namespace Math {

// 3D gradient noise after Perlin 1985 "An Image Synthesizer".
// Returns approximately [-1, 1]. Backed by stb_perlin.
float Perlin3D(float x, float y, float z);

}  // namespace Math

#endif  // PERLIN_H