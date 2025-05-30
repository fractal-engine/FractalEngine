#ifndef CAMERA_SYSTEM_H
#define CAMERA_SYSTEM_H

#include <SDL.h>
#include "editor/components/orbit_camera.h"

class CameraSystem {
public:
  explicit CameraSystem(OrbitCamera* camera) : camera_(camera) {}

  void UpdateFromKeyboard() {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    if (keys[SDL_SCANCODE_W])
      camera_->pan(0.0f, 70.30f);
    if (keys[SDL_SCANCODE_S])
      camera_->pan(0.0f, -70.30f);
    if (keys[SDL_SCANCODE_A])
      camera_->pan(-70.30f, 0.0f);
    if (keys[SDL_SCANCODE_D])
      camera_->pan(70.30f, 0.0f);
  }

  void Reset() {
    camera_->setDistance(100.0f);
    camera_->setPitch(0.5f);
    camera_->setYaw(0.0f);
    float resetTarget[3] = {77.5f, 0.0f, 77.5f};
    camera_->setTarget(resetTarget);
  }

  void GetViewMatrix(float* out) const { camera_->getViewMatrix(out); }
  void GetProjectionMatrix(float* out, float aspect, float fov = 60.0f) const {
    camera_->getProjectionMatrix(out, aspect, fov);
  }

private:
  OrbitCamera* camera_;
};

#endif  // CAMERA_SYSTEM_H
