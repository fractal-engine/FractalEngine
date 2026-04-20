#ifndef PROCMODEL_VALIDATION_RESULT_H
#define PROCMODEL_VALIDATION_RESULT_H

#include <cstdint>
#include <glm/vec3.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace ProcModel {

// Single diagnostic issue emitted by a check. Multiple diagnostics may be
// produced for a single instance; the overall pass/fail is determined by
// whether any diagnostic has Severity::Error.
struct Diagnostic {
  enum class Severity { Warning, Error };

  // Stable string codes. Kept as string (not enum) so new checks can be
  // added without touching every call site; analysis scripts can filter
  // on these directly.
  // Current codes:
  //   "constraint.excludes"        - two mutually-excluded parts both selected
  //   "constraint.requires"        - REQUIRES rule unsatisfied
  //   "attachment.duplicate"       - more than one part resolved to same attach
  //   node "attachment.dangling"        - attach_to references a node that
  //   doesn't exist "attachment.orphaned"        - part attached but its parent
  //   base wasn't selected "group.missing_activation"   - required group failed
  //   to activate "group.forward_misaligned"   - attachment forward axes
  //   diverge within a group
  std::string code;
  Severity severity = Severity::Error;

  // Part IDs involved in the violation (may be empty for group-level issues).
  std::vector<std::string> part_ids;

  // Group ID context, if applicable.
  std::string group_id;

  // Human-readable message for debugging. Not intended for analysis.
  std::string message;
};

// Axis-aligned bounding box in local model space.
struct AABB {
  glm::vec3 min{0.0f};
  glm::vec3 max{0.0f};
  bool valid = false;  // false if no geometry contributed
};

// Raw per-instance record. This is appended to variation_log.jsonl, one
// record per Generate() attempt (whether it passed or failed validation).
// All fields are raw data; derived metrics (structural diff, parametric
// deviation, ERA coverage) are computed at analysis time from many records.
struct ValidationResult {
  // Identity
  std::string model_id;
  uint64_t seed = 0;
  int attempt_index = 0;  // Which attempt within the retry loop produced this
  int64_t timestamp_ms = 0;  // Unix epoch ms at validation time

  // Outcome
  bool passed = false;  // true iff no Severity::Error diagnostics
  std::vector<Diagnostic> diagnostics;

  // Raw selection data (for structural variation analysis)
  std::vector<std::string> selected_part_ids;
  std::vector<std::string> active_group_ids;

  // Raw parameter data per resolved descriptor (for parametric variation)
  struct ResolvedEntry {
    std::string descriptor_id;
    std::string group_id;
    glm::vec3 applied_rotation{0.0f};
    glm::vec3 applied_scale{1.0f};
    std::vector<std::string> attach_to;
  };
  std::vector<ResolvedEntry> resolved_entries;

  // Geometry
  AABB model_bounds;
  std::unordered_map<std::string, AABB>
      per_part_bounds;  // keyed by descriptor_id
  std::unordered_map<std::string, AABB> per_group_bounds;  // keyed by group_id
};

}  // namespace ProcModel

#endif  // PROCMODEL_VALIDATION_RESULT_H