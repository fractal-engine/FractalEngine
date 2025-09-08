#include "time.h"

namespace Time {

double g_time = 0.0;
double g_last_time = 0.0;
double g_unscaled_delta = 0.0;
double g_delta = 0.0;
double g_time_scale = 1.0;

// Take delta time directly
void Step(double delta_time) {
  g_unscaled_delta = delta_time;
  g_delta = g_unscaled_delta * g_time_scale;
  g_time += delta_time;
  g_last_time = g_time - delta_time;
}

double Now() {
  return g_time;
}

float Nowf() {
  return static_cast<float>(g_time);
}

double Last() {
  return g_last_time;
}

float Lastf() {
  return static_cast<float>(g_last_time);
}

double Delta() {
  return g_delta;
}

float Deltaf() {
  return static_cast<float>(g_delta);
}

double UnscaledDelta() {
  return g_unscaled_delta;
}

double UnscaledDeltaf() {
  return static_cast<float>(g_unscaled_delta);
}

double GetTimeScale() {
  return g_time_scale;
}

void SetTimeScale(double time_scale) {
  g_time_scale = time_scale;
}

}  // namespace Time