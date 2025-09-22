#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "engine/ecs/ecs_collection.h"

enum class Space { LOCAL, WORLD };

namespace Transform {

// ──────────────── CORE FUNCTIONS ────────────────

bool IsRoot(const TransformComponent& transform);
bool HasParent(const TransformComponent& transform);

// Get parent; transform should have a parent before using this
TransformComponent& GetParent(const TransformComponent& transform);

// Update model matrix (local or with parent)
void Evaluate(TransformComponent& transform);
void Evaluate(TransformComponent& transform, const TransformComponent& parent);

// Update MVP matrix
void UpdateMVP(TransformComponent& transform, const glm::mat4& viewProjection);

// ──────────────── TRANSFORMATION ────────────────
void SetPosition(TransformComponent& transform, const glm::vec3& position,
                 Space space = Space::LOCAL);
void SetRotation(TransformComponent& transform, const glm::quat& rotation,
                 Space space = Space::LOCAL);
void SetEulerAngles(TransformComponent& transform, const glm::vec3& eulerAngles,
                    Space space = Space::LOCAL);
void SetScale(TransformComponent& transform, const glm::vec3& scale,
              Space space = Space::LOCAL);

glm::vec3 GetPosition(const TransformComponent& transform,
                      Space space = Space::LOCAL);
glm::quat GetRotation(const TransformComponent& transform,
                      Space space = Space::LOCAL);
glm::vec3 GetEulerAngles(const TransformComponent& transform,
                         Space space = Space::LOCAL);
glm::vec3 GetScale(const TransformComponent& transform,
                   Space space = Space::LOCAL);

void Translate(TransformComponent& transform, const glm::vec3& delta,
               Space space = Space::LOCAL);
void Rotate(TransformComponent& transform, const glm::quat& delta,
            Space space = Space::LOCAL);
void Scale(TransformComponent& transform, const glm::vec3& scale,
           Space space = Space::LOCAL);

// ──────────────── HELPERS ────────────────
glm::vec3 Forward(const TransformComponent& transform,
                  Space space = Space::LOCAL);
glm::vec3 Backward(const TransformComponent& transform,
                   Space space = Space::LOCAL);
glm::vec3 Right(const TransformComponent& transform,
                Space space = Space::LOCAL);
glm::vec3 Left(const TransformComponent& transform, Space space = Space::LOCAL);
glm::vec3 Up(const TransformComponent& transform, Space space = Space::LOCAL);
glm::vec3 Down(const TransformComponent& transform, Space space = Space::LOCAL);

// Quaternion for rotating from position to target
glm::quat LookAt(const glm::vec3& position, const glm::vec3& target);

// Euler <-> Quaternion conversions (degrees)
glm::quat ToQuat(const glm::vec3& eulerAngles);
glm::vec3 ToEuler(const glm::quat& quaternion);

}  // namespace Transform

#endif  // TRANSFORM_H
