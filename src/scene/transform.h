#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <bx/math.h>

struct Transform {
  float position[3] = {0, 0, 0};
  float rotation[3] = {0, 0, 0};  // radians
  float scale[3] = {1, 1, 1};

  void GetMatrix(float* out) const {
    bx::mtxSRT(out, scale[0], scale[1], scale[2], rotation[0], rotation[1],
               rotation[2], position[0], position[1], position[2]);
  }
};

#endif  // TRANSFORM_H