#ifndef CAMERA_H
#define CAMERA_H

#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "engine/ecs/components/camera_component.h"
#include "engine/ecs/components/transform_component.h"
#include "engine/math/transformation.h"

class CameraView {
public:
  static void GetViewMatrix(const TransformComponent& transform, float* out) {
    glm::mat4 view =
        Transformation::View(transform.position_, transform.rotation_);
    memcpy(out, glm::value_ptr(view), 16 * sizeof(float));
  }

  static void GetProjectionMatrix(const CameraComponent& camera, float* out,
                                  float aspect) {
    glm::mat4 projection = Transformation::Projection(
        camera.fov_, aspect, camera.near_clip_, camera.far_clip_);
    memcpy(out, glm::value_ptr(projection), 16 * sizeof(float));
  }
};

#endif  // CAMERA_H