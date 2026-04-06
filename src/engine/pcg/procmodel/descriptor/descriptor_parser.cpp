#include "descriptor_parser.h"

#include "descriptor_parser.h"

#include <fstream>

#include "engine/core/logger.h"
#include "engine/pcg/procmodel/descriptor/model_descriptor.h"
#include "glm/ext/vector_float3.hpp"

namespace ProcModel {

bool DescriptorParser::ParseSelectionGroup(const nlohmann::json& j,
                                           SelectionGroup& out) {
  if (!j.contains("group_id") || !j.contains("parts"))
    return false;

  out.group_id = j["group_id"].get<std::string>();
  out.required = j.value("required", true);
  out.activated_by = j.value("activated_by", std::string(""));

  if (j.contains("attach_to")) {
    for (const auto& id : j["attach_to"]) {
      out.attach_to.push_back(id.get<std::string>());
    }
  }

  for (const auto& part_json : j["parts"]) {
    PartDescriptor part;
    part.id = part_json["id"].get<std::string>();
    part.name = part_json.value("name", part.id);
    part.weight = part_json.value("weight", 1.0f);
    out.parts.push_back(std::move(part));
  }

  return !out.parts.empty();
}

bool DescriptorParser::ParseParameterRange(const nlohmann::json& j,
                                           ParameterRange& out) {
  if (!j.contains("part_id"))
    return false;

  out.part_id = j["part_id"].get<std::string>();
  out.activated_by = j.value("activated_by", std::string(""));

  if (j.contains("rotation_min")) {
    auto& r = j["rotation_min"];
    out.rotation_min = glm::vec3(r[0], r[1], r[2]);
  }
  if (j.contains("rotation_max")) {
    auto& r = j["rotation_max"];
    out.rotation_max = glm::vec3(r[0], r[1], r[2]);
  }

  return true;
}

bool DescriptorParser::ParseConstraintRule(const nlohmann::json& j,
                                           ConstraintRule& out) {
  if (!j.contains("id") || !j.contains("type") || !j.contains("part_a") ||
      !j.contains("part_b"))
    return false;

  out.id = j["id"].get<std::string>();
  out.part_a = j["part_a"].get<std::string>();
  out.part_b = j["part_b"].get<std::string>();

  std::string type_str = j["type"].get<std::string>();
  if (type_str == "excludes") {
    out.type = ConstraintRule::Type::EXCLUDES;
  } else if (type_str == "requires") {
    out.type = ConstraintRule::Type::REQUIRES;
  } else {
    return false;
  }

  return true;
}

bool DescriptorParser::ParseParameterBinding(const nlohmann::json& j,
                                             ParameterBinding& out) {
  if (!j.contains("source_part") || !j.contains("target_part"))
    return false;

  out.source_part = j["source_part"].get<std::string>();
  out.source_param = j.value("source_param", std::string(""));
  out.target_part = j["target_part"].get<std::string>();
  out.target_param = j.value("target_param", std::string(""));
  out.ratio = j.value("ratio", 1.0f);

  return true;
}

bool DescriptorParser::FromJson(const nlohmann::json& j, ModelDescriptor& out) {
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

  if (j.contains("parameter_ranges")) {
    for (const auto& range_json : j["parameter_ranges"]) {
      ParameterRange range;
      if (ParseParameterRange(range_json, range)) {
        out.parameter_ranges.push_back(std::move(range));
      } else {
        Logger::getInstance().Log(
            LogLevel::Warning,
            "[DescriptorParser] Failed to parse parameter range");
      }
    }
  }

  if (j.contains("constraints")) {
    for (const auto& rule_json : j["constraints"]) {
      ConstraintRule rule;
      if (ParseConstraintRule(rule_json, rule)) {
        out.constraints.push_back(std::move(rule));
      } else {
        Logger::getInstance().Log(
            LogLevel::Warning,
            "[DescriptorParser] Failed to parse constraint rule");
      }
    }
  }

  if (j.contains("parameter_bindings")) {
    for (const auto& binding_json : j["parameter_bindings"]) {
      ParameterBinding binding;
      if (ParseParameterBinding(binding_json, binding)) {
        out.parameter_bindings.push_back(std::move(binding));
      } else {
        Logger::getInstance().Log(
            LogLevel::Warning,
            "[DescriptorParser] Failed to parse parameter binding");
      }
    }
  }

  return true;
}

bool DescriptorParser::LoadFromFile(const std::string& path,
                                    ModelDescriptor& out) {
  try {
    std::ifstream file(path);
    if (!file.is_open()) {
      Logger::getInstance().Log(LogLevel::Error,
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
