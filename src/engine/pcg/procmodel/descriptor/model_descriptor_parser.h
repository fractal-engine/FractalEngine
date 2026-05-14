#ifndef MODEL_DESCRIPTOR_PARSER_H
#define MODEL_DESCRIPTOR_PARSER_H

#include <nlohmann/json.hpp>
#include <string>

#include "engine/pcg/procmodel/descriptor/model_descriptor.h"

namespace ProcModel {

class ModelDescriptorParser {
public:
  static bool FromJson(const nlohmann::json& j, ModelDescriptor& out);

private:
  static bool ParseSelectionGroup(const nlohmann::json& j, SelectionGroup& out);
  static bool ParseTransformRange(const nlohmann::json& j, TransformRange& out);
  static bool ParseConstraintRule(const nlohmann::json& j, ConstraintRule& out);
  static bool ParseParameterBinding(const nlohmann::json& j,
                                    ParameterBinding& out);
};

}  // namespace ProcModel

#endif  // MODEL_DESCRIPTOR_PARSER_H
