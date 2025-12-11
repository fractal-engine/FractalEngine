#ifndef CONSTRAINT_SYSTEM_H
#define CONSTRAINT_SYSTEM_H

#include <string>
#include <unordered_map>
#include <vector>

namespace PCG {

// constraint: condition that must be met
struct Constraint {
  std::string property_name;
  float min_value;
  float max_value;

  bool IsSatisfied(float value) const {
    return value >= min_value && value <= max_value;
  }
};

struct Rule {
  std::vector<Constraint> constraints;  // all must be satisfied
  std::unordered_map<std::string, std::vector<float>>
      outputs;       // e.g. color -> rgba
  int priority = 0;  // higher priority wins if multiple rules match
};
struct Properties {
  std::unordered_map<std::string, float> values;
};

class ConstraintSystem {
public:
  void AddRule(const Rule& rule) { rules_.push_back(rule); }
  const Rule* Match(const Properties&) const;

  size_t GetRuleCount() const { return rules_.size(); }

private:
  std::vector<Rule> rules_;
};

}  // namespace PCG

#endif  // CONSTRAINT_SYSTEM_H