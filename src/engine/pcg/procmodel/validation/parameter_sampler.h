#ifndef PARAMETER_SAMPLER_H
#define PARAMETER_SAMPLER_H

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <pcg_random.hpp>

#include "procmodel_analysis.h"

namespace ProcModel {

class ParameterSampler {
public:
  // Mark which descriptor subsequent Uniform() calls attribute to.
  // Pass empty string to attribute to the instance bucket instead.
  void Set(const std::string& descriptor_id);

  // Sample under the current descriptor.
  float Uniform(const std::string& op_kind, const std::string& axis, float lo,
                float hi, pcg32& rng);

  // Sample under the instance-wide bucket, regardless of current descriptor.
  float UniformInstance(const std::string& op_kind, const std::string& axis,
                        float lo, float hi, pcg32& rng);

  // Variant for optional ranges.
  std::optional<float> UniformOptional(const std::string& op_kind,
                                       const std::string& axis,
                                       const std::optional<float>& lo,
                                       const std::optional<float>& hi,
                                       pcg32& rng);

  // Read accumulated samples (used by validator).
  const std::unordered_map<std::string, std::vector<ParameterSample>>&
  ByDescriptor() const {
    return by_descriptor_;
  }
  const std::vector<ParameterSample>& Instance() const { return instance_; }

private:
  std::string current_id_;
  std::unordered_map<std::string, std::vector<ParameterSample>> by_descriptor_;
  std::vector<ParameterSample> instance_;
};

}  // namespace ProcModel

#endif  // PARAMETER_SAMPLER_H