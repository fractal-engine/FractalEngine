#ifndef PROCMODEL_ANALYSIS_H
#define PROCMODEL_ANALYSIS_H

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
struct ProcModelSample {
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
  struct PartSample {
    std::string descriptor_id;
    std::string group_id;
    glm::vec3 applied_rotation{0.0f};
    glm::vec3 applied_scale{1.0f};
    std::vector<std::string> attach_to;
  };
  std::vector<PartSample> part_samples;

  // Geometry
  AABB model_bounds;
  std::unordered_map<std::string, AABB>
      per_part_bounds;  // keyed by descriptor_id
  std::unordered_map<std::string, AABB> per_group_bounds;  // keyed by group_id
};

// ERA metrics - computed across a sample of N instances.
// Follows Smith & Whitehead (2010) / Karth (2019) expressive range analysis.
// Populated by ERAAnalyzer::Analyze(), not per-instance validation.
struct ProcModelERA {
  // Total instances in the sample
  int sample_size = 0;

  // Structural diversity
  // Number of unique part-selection combinations - unique phenotypes
  int unique_combination_count = 0;
  // unique_combination_count / sample_size. 1.0 = no duplicates.
  float combination_coverage = 0.0f;
  // Count of exact duplicate instances - same selected_part_ids set
  int exact_duplicate_count = 0;

  // Per-group selection frequency
  // group_id -> (part_id -> selection count)
  std::unordered_map<std::string, std::unordered_map<std::string, int>>
      part_frequency;

  // Active group count distribution
  // How many groups fired per instance: min, max, mean
  int min_active_groups = 0;
  int max_active_groups = 0;
  float mean_active_groups = 0.0f;

  // Geometric range
  // Distribution of model AABB volume across instances: min, max, mean
  float min_model_volume = 0.0f;
  float max_model_volume = 0.0f;
  float mean_model_volume = 0.0f;
};

}  // namespace ProcModel

#endif  // PROCMODEL_ANALYSIS_H