#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>

struct CameraComponent {
  // Set if camera is enabled
  bool enabled = true;

  // Cameras y fov in degrees
  float fov = 70.0f;

  // Near clipping
  float near_clip = 0.3f;

  // Far clipping
  float far_clip = 1000.0f;
};

#endif  // CAMERA_H
