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
  float weight;
};

struct SelectionGroup {
  std::string group_id;
  std::vector<PartDescriptor> parts;
  bool required = true;
  std::string activated_by;            // Empty = root group
  std::vector<std::string> attach_to;  // attachment slots

  // Scale inherited from the activator (parent). If unset, no inheritance.
  // A value of 0.6 means: this group's parts are 60% the size of their parent.
  std::optional<float> hierarchy_scale_factor;
  std::optional<float> hierarchy_scale_jitter;  // ± randomness on the factor

  // If true: each attach point in attach_to re-runs WeightedSelect, allowing
  // different parts at different attachments (organic variation - branches,
  // leaves). If false (default): one part is selected for the group and
  // duplicated to every attachment (uniform assembly — columns, pillars).
  bool select_per_attachment = false;

  // Per-attachment rotation jitter. When non-zero, each attached instance
  // receives an independent random rotation perturbation drawn uniformly
  // from [-jitter, +jitter] on each axis (radians at runtime, degrees in
  // JSON). Default zero: no jitter, attachments inherit the activator's
  // base rotation unchanged.
  glm::vec3 rotation_jitter = glm::vec3(0.0f);
};

//
// TRANSFORM RANGE
// used for per-part transforms
//
struct TransformRange {
  std::string part_id;

  std::optional<glm::vec3> scale_min;
  std::optional<glm::vec3> scale_max;
  std::optional<glm::vec3> rotation_min;
  std::optional<glm::vec3> rotation_max;
};

struct DeformationRange {
  std::string group_id;

  std::optional<float> taper_factor_min;
  std::optional<float> taper_factor_max;

  std::optional<float> twist_angle_min;
  std::optional<float> twist_angle_max;

  std::optional<float> bend_angle_min;
  std::optional<float> bend_angle_max;

  std::optional<float> noise_amplitude_min;
  std::optional<float> noise_amplitude_max;
};

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
  std::string model_id;  // archetype identity
  std::string model_name;
  std::string path;
  std::string domain;  // e.g. vegetation, building, etc

  std::vector<SelectionGroup> selection_groups;
  std::vector<TransformRange> transform_ranges;
  std::vector<ConstraintRule> constraints;
  std::vector<ParameterBinding> parameter_bindings;
  std::vector<DeformationRange> part_deformation_ranges;
  std::optional<DeformationRange> model_deformation_range;

  // Archetype-level attributes
  std::optional<glm::vec3> scale_min;  // model scale range
  std::optional<glm::vec3> scale_max;
  std::vector<std::string> tags;  // ex: "vegetation", "tropical"
};

}  // namespace ProcModel

#endif  // MODEL_DESCRIPTOR_H
