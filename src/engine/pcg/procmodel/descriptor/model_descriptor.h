#ifndef MODEL_DESCRIPTOR_H
#define MODEL_DESCRIPTOR_H

#include <cstdint>
#include <glm/vec3.hpp>
#include <optional>
#include <string>
#include <vector>

namespace ProcModel {

struct PartDescriptor {
  std::string id;
  std::string name;
  float weight = 1.0f;
};

// used to be selection groups
struct SelectionGroup {
  std::string group_id;
  std::vector<PartDescriptor> parts;
  bool required = true;
  std::string activated_by;  // Empty = root group
};

struct ParameterRange {
  std::string part_id;

  // std::optional<glm::vec3> scale_min;
  // std::optional<glm::vec3> scale_max;
  std::optional<glm::vec3> rotation_min;
  std::optional<glm::vec3> rotation_max;

  std::string activated_by;  // Empty = always applied to this node if selected
};

// used to be ConstraintRule
struct ConstraintRule {
  enum class Type { EXCLUDES, REQUIRES };

  std::string id;
  Type type;
  std::string part_a;
  std::string part_b;
};

struct ParameterBinding {
  std::string source_part;
  std::string source_param;
  std::string target_part;
  std::string target_param;
  float ratio = 1.0f;
};

// Full archetype definition
struct ModelDescriptor {
  std::string model_id; // archetype identity
  std::string model_name;
  std::string path;
  std::string domain; // e.g. vegetation, building, etc

  std::vector<SelectionGroup> selection_groups;
  std::vector<ParameterRange> parameter_ranges;
  std::vector<ConstraintRule> constraints;
  std::vector<ParameterBinding> parameter_bindings;

  // Archetype-level attributes
  std::optional<glm::vec3> scale_min;  // model scale range
  std::optional<glm::vec3> scale_max;
  std::vector<std::string> tags;       // ex: "vegetation", "tropical"
};

}  // namespace ProcModel

#endif  // MODEL_DESCRIPTOR_H
