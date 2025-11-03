#ifndef REMAP_H
#define REMAP_H

namespace Generator {

struct TerracingParams {
  int steps = 5;
  float smoothness = 0.0f;
};

struct PlateauParams {
  float threshold = 0.6f;
  float smoothness = 0.1f;  // Transition width
};

class Remap {
public:
  static float Terrace(float value, const TerracingParams&) { return value; }
  static float Plateau(float value, const PlateauParams&) { return value; }
};

}  // namespace Generator

#endif  // REMAP_H