#ifndef RIDGE_H
#define RIDGE_H

namespace Generator {

class Ridge {
public:
  // lfSharpness: 0.0 = billow (soft), 1.0 = ridge (sharp)
  static float Blend(float value, float sharpness);
};

}  // namespace Generator

#endif  // RIDGE_H