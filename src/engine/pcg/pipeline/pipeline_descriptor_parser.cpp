#include "pipeline_descriptor_parser.h"

#include <nlohmann/json.hpp>
#include <string>

#include "engine/core/logger.h"
#include "engine/pcg/pipeline/pipeline_descriptor.h"

namespace PCG {

bool ParsePipelineDescriptor(const nlohmann::json& j, PipelineDescriptor& out) {
  if (!j.contains("pipeline"))
    return true;

  const auto& pipeline_json = j["pipeline"];
  if (!pipeline_json.is_array()) {
    Logger::getInstance().Log(
        LogLevel::Warning,
        "[PCG::ParsePipelineDescriptor] 'pipeline' is not an array");
    return true;
  }

  for (const auto& entry_json : pipeline_json) {
    if (!entry_json.contains("kind")) {
      Logger::getInstance().Log(
          LogLevel::Warning,
          "[PCG::ParsePipelineDescriptor] entry missing 'kind', skipping");
      continue;
    }
    PipelineEntry entry;
    entry.kind = entry_json["kind"].get<std::string>();
    entry.params = entry_json.value("params", nlohmann::json::object());
    out.entries.push_back(std::move(entry));
  }

  return true;
}

}  // namespace PCG