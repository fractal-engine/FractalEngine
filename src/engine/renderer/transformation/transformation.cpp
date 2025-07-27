#include "transformation.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Transformation {

glm::vec3 Swap(const glm::vec3& position) {
  return glm::vec3(position.x, position.y, -position.z);
}

glm::quat Swap(const glm::quat& rotation) {
  return glm::quat(rotation.w, -rotation.x, -rotation.y, rotation.z);
}

glm::mat4 Model(const glm::vec3& position, const glm::quat& rotation,
                const glm::vec3& scale) {
  glm::mat4 model(1.0f);
  model = glm::translate(model, Swap(position));
  model *= glm::mat4_cast(glm::normalize(Swap(rotation)));
  model = glm::scale(model, scale);
  return model;
}

glm::mat4 View(const glm::vec3& camera_position,
               const glm::quat& camera_rotation) {
  glm::vec3 position = Swap(camera_position);
  glm::quat rotation = glm::normalize(Swap(camera_rotation));
  glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, -1.0f);
  glm::vec3 up = rotation * glm::vec3(0.0f, 1.0f, 0.0f);
  return glm::lookAt(position, position + forward, up);
}

glm::mat4 Projection(float fov, float aspect, float near, float far) {
  return glm::perspective(glm::radians(fov), aspect, near, far);
}

glm::mat4 Normal(const glm::mat4& model) {
  return glm::transpose(glm::inverse(glm::mat3(model)));
}

}  // namespace Transformation