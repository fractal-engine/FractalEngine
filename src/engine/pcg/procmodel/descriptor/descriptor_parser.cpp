#include "descriptor_parser.h"

#include "descriptor_parser.h"

#include <fstream>

#include "engine/core/logger.h"

namespace ProcModel {

bool DescriptorParser::ParseSelectionGroup(const nlohmann::json& j,
                                           SelectionGroup& out) {
  if (!j.contains("group_id") || !j.contains("parts"))
    return false;

  out.group_id = j["group_id"].get<std::string>();
  out.required = j.value("required", true);
  out.activated_by = j.value("activated_by", std::string(""));

  for (const auto& part_json : j["parts"]) {
    PartDescriptor part;
    part.id = part_json["id"].get<std::string>();
    part.name = part_json.value("name", part.id);
    part.weight = part_json.value("weight", 1.0f);
    out.parts.push_back(std::move(part));
  }

  return !out.parts.empty();
}

bool DescriptorParser::FromJson(const nlohmann::json& j,
                                ModelDescriptor& out) {
  if (!j.contains("model_id")) {
    Logger::getInstance().Log(LogLevel::Error,
                              "[DescriptorParser] Missing model_id");
    return false;
  }

  out.model_id = j["model_id"].get<std::string>();
  out.model_name = j.value("model_name", out.model_id);
  out.domain = j.value("domain", std::string(""));
  out.path = j.value("path", std::string(""));

  // Whole-model scale range
  if (j.contains("scale_min")) {
    auto& s = j["scale_min"];
    out.scale_min = glm::vec3(s[0], s[1], s[2]);
  }
  if (j.contains("scale_max")) {
    auto& s = j["scale_max"];
    out.scale_max = glm::vec3(s[0], s[1], s[2]);
  }

  // Selection groups
  if (j.contains("selection_groups")) {
    for (const auto& group_json : j["selection_groups"]) {
      SelectionGroup group;
      if (ParseSelectionGroup(group_json, group)) {
        out.selection_groups.push_back(std::move(group));
      } else {
        Logger::getInstance().Log(
            LogLevel::Warning,
            "[DescriptorParser] Failed to parse selection group");
      }
    }
  }

  // TODO: ParseParameterRange, ParseConstraintRule, ParseParameterBinding

  return true;
}

bool DescriptorParser::LoadFromFile(const std::string& path,
                                    ModelDescriptor& out) {
  try {
    std::ifstream file(path);
    if (!file.is_open()) {
      Logger::getInstance().Log(
          LogLevel::Error,
          "[DescriptorParser] Cannot open: " + path);
      return false;
    }
    nlohmann::json j = nlohmann::json::parse(file);
    return FromJson(j, out);
  } catch (const std::exception& e) {
    Logger::getInstance().Log(
        LogLevel::Error,
        "[DescriptorParser] Parse error: " + std::string(e.what()));
    return false;
  }
}

}  // namespace ProcModel
