#include "rotation_jitter.h"

#include <nlohmann/json.hpp>
#include <random>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "engine/pcg/procmodel/generator/socket_context.h"

namespace ProcModel {

static std::shared_ptr<PCG::OperationData> ParseSocketRotationJitter(
    const nlohmann::json& params) {
  auto data = std::make_shared<SocketRotationJitterData>();

  const float pitch_deg = params.value("pitch", 0.0f);
  const float yaw_deg = params.value("yaw", 0.0f);
  const float roll_deg = params.value("roll", 0.0f);

  data->range = glm::radians(glm::vec3(pitch_deg, yaw_deg, roll_deg));

  return data;
}

static void ApplySocketRotationJitter(const PCG::OperationData& base_data,
                                      PCG::OperationContext& base_ctx) {
  const auto& data = PCG::OperationCast<SocketRotationJitterData>(base_data);
  auto& ctx = PCG::OperationCast<SocketContext>(base_ctx);

  // Draw three independent uniform angles within the authored range.
  auto draw = [&](float r) -> float {
    if (r <= 0.0f)
      return 0.0f;
    std::uniform_real_distribution<float> dist(-r, r);
    return dist(ctx.rng);
  };
  const float pitch = draw(data.range.x);
  const float yaw = draw(data.range.y);
  const float roll = draw(data.range.z);

  // Compose as pitch (X) → yaw (Y) → roll (Z) intrinsic rotations.
  // Post-multiplied into socket_world so jitter applies in the socket's
  // own local frame; pre-multiply would jitter in world space, which
  // would defeat radial_align if listed after it.
  glm::mat4 jitter(1.0f);
  jitter = glm::rotate(jitter, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
  jitter = glm::rotate(jitter, pitch, glm::vec3(1.0f, 0.0f, 0.0f));
  jitter = glm::rotate(jitter, roll, glm::vec3(0.0f, 0.0f, 1.0f));

  ctx.socket_world = ctx.socket_world * jitter;
}

void RegisterSocketRotationJitterOperation(PCG::OperationRegistry& registry) {
  registry.Register("new_rotation_jitter", &ParseSocketRotationJitter,
                    &ApplySocketRotationJitter);
}

}  // namespace ProcModel