#ifndef DESCRIPTOR_PARSER_H
#define DESCRIPTOR_PARSER_H

#include <nlohmann/json.hpp>
#include <string>

#include "engine/pcg/procmodel/descriptor/model_descriptor.h"

namespace ProcModel {

class DescriptorParser {
 public:
  static bool FromJson(const nlohmann::json& j, ModelDescriptor& out);
  static bool LoadFromFile(const std::string& path, ModelDescriptor& out);

 private:
  static bool ParseSelectionGroup(const nlohmann::json& j, SelectionGroup& out);
  static bool ParseParameterRange(const nlohmann::json& j, ParameterRange& out);
  static bool ParseConstraintRule(const nlohmann::json& j, ConstraintRule& out);
  static bool ParseParameterBinding(const nlohmann::json& j, ParameterBinding& out);
};

}  // namespace ProcModel

#endif // DESCRIPTOR_PARSER_H
