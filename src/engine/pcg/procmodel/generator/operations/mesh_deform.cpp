#include "mesh_deform.h"

#include <nlohmann/json.hpp>
#include <random>

#include "engine/core/logger.h"
#include "engine/pcg/pipeline/operation_context.h"
#include "engine/pcg/pipeline/operation_registry.h"
#include "engine/pcg/procmodel/descriptor/model_descriptor.h"
#include "engine/pcg/procmodel/generator/model_context.h"
#include "engine/pcg/procmodel/generator/resolved_model.h"

namespace ProcModel {

static bool ParseVec3(const nlohmann::json& j, const std::string& key,
                      std::optional<glm::vec3>& out) {
  if (!j.contains(key))
    return true;  // missing is fine — leaves optional empty
  const auto& v = j[key];
  if (!v.is_array() || v.size() != 3)
    return false;
  out = glm::vec3(v[0].get<float>(), v[1].get<float>(), v[2].get<float>());
  return true;
}

static std::shared_ptr<PCG::OperationData> ParseMeshDeform(
    const nlohmann::json& params) {
  auto data = std::make_shared<MeshDeformData>();

  if (!ParseVec3(params, "rotation_jitter_min", data->rotation_jitter_min) ||
      !ParseVec3(params, "rotation_jitter_max", data->rotation_jitter_max) ||
      !ParseVec3(params, "scale_jitter_min", data->scale_jitter_min) ||
      !ParseVec3(params, "scale_jitter_max", data->scale_jitter_max)) {
    Logger::getInstance().Log(
        LogLevel::Error, "[MeshDeform] invalid parameters in pipeline entry");
    return nullptr;
  }

  // Reject ranges where min > max on any axis.
  auto valid_range = [](const std::optional<glm::vec3>& lo,
                        const std::optional<glm::vec3>& hi) {
    if (!lo || !hi)
      return true;  // partial specifications are silently ignored at apply time
    return lo->x <= hi->x && lo->y <= hi->y && lo->z <= hi->z;
  };
  if (!valid_range(data->rotation_jitter_min, data->rotation_jitter_max) ||
      !valid_range(data->scale_jitter_min, data->scale_jitter_max)) {
    Logger::getInstance().Log(
        LogLevel::Warning,
        "[MeshDeform] mesh_deform params empty — nothing will be applied");
    return nullptr;
  }

  return data;
}

static glm::vec3 SampleVec3Range(const glm::vec3& lo, const glm::vec3& hi,
                                 pcg32& rng) {
  std::uniform_real_distribution<float> dx(lo.x, hi.x);
  std::uniform_real_distribution<float> dy(lo.y, hi.y);
  std::uniform_real_distribution<float> dz(lo.z, hi.z);
  return glm::vec3(dx(rng), dy(rng), dz(rng));
}

// Per-instance independent jitter
// correlation comes from the ECS transform hierarchy automatically.
static void ApplyMeshDeform(const PCG::OperationData& base_data,
                            PCG::OperationContext& base_ctx) {
  const auto& data = PCG::OperationCast<MeshDeformData>(base_data);
  auto& ctx = PCG::OperationCast<ModelContext>(base_ctx);

  for (auto& d : ctx.model.descriptors) {
    if (data.rotation_jitter_min && data.rotation_jitter_max) {
      d.applied_rotation += SampleVec3Range(*data.rotation_jitter_min,
                                            *data.rotation_jitter_max, ctx.rng);
    }
    if (data.scale_jitter_min && data.scale_jitter_max) {
      d.applied_scale *= SampleVec3Range(*data.scale_jitter_min,
                                         *data.scale_jitter_max, ctx.rng);
    }
  }
}

void RegisterMeshDeformOperation(PCG::OperationRegistry& registry) {
  registry.Register("mesh_deform", &ParseMeshDeform, &ApplyMeshDeform);
}

}  // namespace ProcModel