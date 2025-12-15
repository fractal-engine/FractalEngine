#include "constraint_system.h"

namespace PCG {
const Rule* ConstraintSystem::Match(const Properties& props) const {

  const Rule* best_match = nullptr;
  int highest_priority = -999999;

  for (const Rule& rule : rules_) {
    // Check if ALL constraints are satisfied
    bool all_satisfied = true;

    for (const Constraint& constraint : rule.constraints) {
      auto it = props.values.find(constraint.property_name);
      if (it == props.values.end()) {
        all_satisfied = false;
        break;
      }

      if (!constraint.IsSatisfied(it->second)) {
        all_satisfied = false;
        break;
      }
    }

    // If this rule matches and has higher priority, use it
    if (all_satisfied && rule.priority > highest_priority) {
      best_match = &rule;
      highest_priority = rule.priority;
    }
  }

  return best_match;
}

}  // namespace PCG
