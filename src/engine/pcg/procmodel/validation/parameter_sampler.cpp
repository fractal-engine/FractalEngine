#include "parameter_sampler.h"

#include <random>

namespace ProcModel {

void ParameterSampler::Set(const std::string& id) {
  current_id_ = id;
}

float ParameterSampler::Uniform(const std::string& op_kind,
                                const std::string& axis, float lo, float hi,
                                pcg32& rng) {
  float sample;
  if (lo == hi) {
    sample = lo;
  } else {
    std::uniform_real_distribution<float> dist(lo, hi);
    sample = dist(rng);
  }
  by_descriptor_[current_id_].push_back({op_kind, axis, sample, lo, hi});
  return sample;
}

float ParameterSampler::UniformInstance(const std::string& op_kind,
                                        const std::string& axis, float lo,
                                        float hi, pcg32& rng) {
  float sample;
  if (lo == hi) {
    sample = lo;
  } else {
    std::uniform_real_distribution<float> dist(lo, hi);
    sample = dist(rng);
  }
  instance_.push_back({op_kind, axis, sample, lo, hi});
  return sample;
}

}  // namespace ProcModel