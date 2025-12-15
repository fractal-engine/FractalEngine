#ifndef TRANSFORMATION_H
#define TRANSFORMATION_H

#include <glm/glm.hpp>

namespace Transformation {
// Converts a position between backend and user coordinates
glm::vec3 Swap(const glm::vec3& position);

// Converts a rotation between backend and user coordinates
glm::quat Swap(const glm::quat& rotation);

// Evaluates model matrix
glm::mat4 Model(const glm::vec3& position, const glm::quat& rotation,
                const glm::vec3& scale);

// Evaluates view matrix
glm::mat4 View(const glm::vec3& cameraPosition,
               const glm::quat& cameraRotation);

// Evaluates projection matrix
glm::mat4 Projection(float fov, float aspect, float near, float far);

// Evaluates normal matrix (transposed inversed model matrix) of given model
// matrix
glm::mat4 Normal(const glm::mat4& model);
};  // namespace Transformation

#endif  // TRANSFORMATION_H