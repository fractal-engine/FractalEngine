#include "procmodel_evaluation.h"

#include <algorithm>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "engine/content/io/json.h"
#include "engine/core/logger.h"

namespace ProcModel {

static float ComputeAABBVolume(const AABB& box) {
  if (!box.valid)
    return 0.0f;
  glm::vec3 size = box.max - box.min;
  return size.x * size.y * size.z;
}

ProcModelERA ProcModelEval::Compute(
    const std::vector<ProcModelSample>& samples) {
  ProcModelERA era;

  // Filter to passed samples only
  std::vector<const ProcModelSample*> passed;
  for (const auto& s : samples) {
    if (s.passed)
      passed.push_back(&s);
  }

  era.sample_size = static_cast<int>(passed.size());
  if (passed.empty())
    return era;

  // --- Structural diversity ---
  // Canonical key: sorted part IDs joined into a string
  std::unordered_map<std::string, int> combination_counts;
  for (const auto* s : passed) {
    std::string key;
    for (const auto& id : s->selected_part_ids) {
      key += id + '|';
    }
    ++combination_counts[key];
  }

  era.unique_combination_count = static_cast<int>(combination_counts.size());
  era.combination_coverage =
      static_cast<float>(era.unique_combination_count) / era.sample_size;

  for (const auto& [_, count] : combination_counts) {
    if (count > 1)
      era.exact_duplicate_count += count - 1;
  }

  // --- Selection frequency ---
  for (const auto* s : passed) {
    for (const auto& part : s->part_samples) {
      ++era.part_frequency[part.group_id][part.descriptor_id];
    }
  }

  // --- Active group distribution ---
  int total_groups = 0;
  era.min_active_groups = INT_MAX;
  era.max_active_groups = 0;

  for (const auto* s : passed) {
    int n = static_cast<int>(s->active_group_ids.size());
    era.min_active_groups = std::min(era.min_active_groups, n);
    era.max_active_groups = std::max(era.max_active_groups, n);
    total_groups += n;
  }

  era.mean_active_groups = static_cast<float>(total_groups) / era.sample_size;

  if (era.min_active_groups == INT_MAX)
    era.min_active_groups = 0;

  // --- Geometric range ---
  float total_volume = 0.0f;
  era.min_model_volume = std::numeric_limits<float>::max();
  era.max_model_volume = 0.0f;

  for (const auto* s : passed) {
    float vol = ComputeAABBVolume(s->model_bounds);
    era.min_model_volume = std::min(era.min_model_volume, vol);
    era.max_model_volume = std::max(era.max_model_volume, vol);
    total_volume += vol;
  }

  era.mean_model_volume = total_volume / era.sample_size;

  if (era.min_model_volume == std::numeric_limits<float>::max())
    era.min_model_volume = 0.0f;

  return era;
}

nlohmann::json SerializeERA(const ProcModelERA& era) {
  nlohmann::json j;
  j["sample_size"] = era.sample_size;
  j["unique_combination_count"] = era.unique_combination_count;
  j["combination_coverage"] = era.combination_coverage;
  j["exact_duplicate_count"] = era.exact_duplicate_count;
  j["min_active_groups"] = era.min_active_groups;
  j["max_active_groups"] = era.max_active_groups;
  j["mean_active_groups"] = era.mean_active_groups;
  j["min_model_volume"] = era.min_model_volume;
  j["max_model_volume"] = era.max_model_volume;
  j["mean_model_volume"] = era.mean_model_volume;

  nlohmann::json freq = nlohmann::json::object();
  for (const auto& [group_id, parts] : era.part_frequency) {
    nlohmann::json group_freq = nlohmann::json::object();
    for (const auto& [part_id, count] : parts)
      group_freq[part_id] = count;
    freq[group_id] = std::move(group_freq);
  }
  j["part_frequency"] = std::move(freq);

  return j;
}

bool ProcModelEval::WriteReport(const ProcModelERA& era,
                                const std::string& path) {
  return Content::WriteJsonFile(path, SerializeERA(era));
}

}  // namespace ProcModel