#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
struct CameraComponent {
  // Set if camera is enabled
  bool enabled_ = true;

  // Cameras y fov in degrees
  float fov_ = 70.0f;

  // Near clipping
  float near_clip_ = 0.3f;

  // Far clipping
  float far_clip_ = 1000.0f;
};

#endif  // CAMERA_H
