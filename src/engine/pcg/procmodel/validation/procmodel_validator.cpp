#include "procmodel_validator.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

#include "engine/core/types/geometry_data.h"

namespace ProcModel {

// Forward axis convention (engine-wide): +Z in local space.
// Transformed by the node's world_transform to produce world-space forward.
static constexpr glm::vec3 kLocalForward = glm::vec3(0.0f, 0.0f, 1.0f);

// Threshold (in radians) beyond which attachment-point forward axes within
// a group are considered misaligned. ~20 degrees.
static constexpr float kForwardAlignmentThresholdRad = 0.35f;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static int64_t NowMs() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch())
      .count();
}

static void ExpandAABB(AABB& box, const glm::vec3& p) {
  if (!box.valid) {
    box.min = p;
    box.max = p;
    box.valid = true;
    return;
  }
  box.min = glm::min(box.min, p);
  box.max = glm::max(box.max, p);
}

static void MergeAABB(AABB& into, const AABB& other) {
  if (!other.valid)
    return;
  if (!into.valid) {
    into = other;
    return;
  }
  into.min = glm::min(into.min, other.min);
  into.max = glm::max(into.max, other.max);
}

// Compute an AABB for a single mesh under a given world transform by
// iterating its CPU-side vertex positions.
// TODO: cache per-mesh local AABBs on ModelGraph to avoid recomputation
// across validation calls.
static AABB ComputeMeshAABB(const Geometry::MeshData& mesh,
                            const glm::mat4& world_transform) {
  AABB box;
  for (const auto& v : mesh.vertices) {
    glm::vec4 p = world_transform * glm::vec4(v.position, 1.0f);
    ExpandAABB(box, glm::vec3(p));
  }
  return box;
}

// ---------------------------------------------------------------------------
// Check: constraint re-verification
//   Catches solver bugs and confirms final selection satisfies all rules.
// ---------------------------------------------------------------------------
static void CheckConstraints(const ResolvedModel& resolved,
                             const ModelDescriptor& descriptor,
                             ValidationResult& out) {
  std::unordered_set<std::string> selected;
  for (const auto& d : resolved.descriptors) {
    // Strip "_at_<node>" suffix produced by multi-attachment expansion so
    // constraint checks operate on the authored part id.
    const std::string& id = d.descriptor_id;
    size_t at = id.find("_at_");
    selected.insert(at == std::string::npos ? id : id.substr(0, at));
  }

  for (const auto& rule : descriptor.constraints) {
    const bool a = selected.count(rule.part_a) > 0;
    const bool b = selected.count(rule.part_b) > 0;

    switch (rule.type) {
      case ConstraintRule::Type::EXCLUDES:
        if (a && b) {
          Diagnostic d;
          d.code = "constraint.excludes";
          d.severity = Diagnostic::Severity::Error;
          d.part_ids = {rule.part_a, rule.part_b};
          d.message = "Mutually-excluded parts both selected";
          out.diagnostics.push_back(std::move(d));
        }
        break;
      case ConstraintRule::Type::REQUIRES:
        if (a != b) {
          Diagnostic d;
          d.code = "constraint.requires";
          d.severity = Diagnostic::Severity::Error;
          d.part_ids = {rule.part_a, rule.part_b};
          d.message = "REQUIRES rule unsatisfied";
          out.diagnostics.push_back(std::move(d));
        }
        break;
    }
  }
}

// ---------------------------------------------------------------------------
// Check: attachment uniqueness + dangling attach_to references
//   Duplicate attachment at the same node is a hard structural bug.
//   Dangling attach_to means the group references an attachment node that
//   doesn't exist in the graph.
// ---------------------------------------------------------------------------
static void CheckAttachments(const ResolvedModel& resolved,
                             const ModelGraph& graph, ValidationResult& out) {
  // Map attach-node-id -> list of descriptors landing there.
  std::unordered_map<std::string, std::vector<std::string>> attach_occupants;

  for (const auto& d : resolved.descriptors) {
    if (d.attach_to.empty())
      continue;

    // descriptor_id is "<part_id>_at_<attach_node>" for attached parts.
    // Extract the concrete attach node name.
    const std::string& id = d.descriptor_id;
    size_t at = id.find("_at_");
    if (at == std::string::npos)
      continue;

    std::string attach_node = id.substr(at + 4);

    if (graph.node_lookup.find(attach_node) == graph.node_lookup.end()) {
      Diagnostic diag;
      diag.code = "attachment.dangling";
      diag.severity = Diagnostic::Severity::Error;
      diag.part_ids = {id};
      diag.message = "attach_to references non-existent node: " + attach_node;
      out.diagnostics.push_back(std::move(diag));
      continue;
    }

    attach_occupants[attach_node].push_back(id);
  }

  for (const auto& [node, occupants] : attach_occupants) {
    if (occupants.size() > 1) {
      Diagnostic diag;
      diag.code = "attachment.duplicate";
      diag.severity = Diagnostic::Severity::Error;
      diag.part_ids = occupants;
      diag.message = "Multiple parts resolved to attachment node: " + node;
      out.diagnostics.push_back(std::move(diag));
    }
  }
}

// ---------------------------------------------------------------------------
// Check: required groups activated
//   Any group marked required must have contributed at least one selection.
// ---------------------------------------------------------------------------
static void CheckGroupActivation(const ResolvedModel& resolved,
                                 const ModelDescriptor& descriptor,
                                 ValidationResult& out) {
  std::unordered_set<std::string> active_groups;
  for (const auto& d : resolved.descriptors) {
    active_groups.insert(d.group_id);
  }
  for (const std::string& g : active_groups) {
    out.active_group_ids.push_back(g);
  }

  // Track which groups *could* have activated. A required group whose
  // activated_by part was never selected is not a violation — its
  // activation is conditional by design.
  std::unordered_set<std::string> selected_parts;
  for (const auto& d : resolved.descriptors) {
    const std::string& id = d.descriptor_id;
    size_t at = id.find("_at_");
    selected_parts.insert(at == std::string::npos ? id : id.substr(0, at));
  }

  for (const auto& group : descriptor.selection_groups) {
    if (!group.required)
      continue;

    const bool is_root = group.activated_by.empty();
    const bool activator_selected =
        !is_root && selected_parts.count(group.activated_by) > 0;

    // Only flag required groups that *should* have activated but didn't.
    if (!is_root && !activator_selected)
      continue;

    if (active_groups.count(group.group_id) == 0) {
      Diagnostic diag;
      diag.code = "group.missing_activation";
      diag.severity = Diagnostic::Severity::Error;
      diag.group_id = group.group_id;
      diag.message = "Required group produced no selection";
      out.diagnostics.push_back(std::move(diag));
    }
  }
}

// ---------------------------------------------------------------------------
// Check: forward axis consistency within a group
//   For each selection group, collect the world-space forward vectors of
//   its attach_to nodes; warn if any pair diverges beyond threshold.
//   Engine convention: +Z is forward (left-handed).
// ---------------------------------------------------------------------------
static void CheckForwardAxisConsistency(const ModelGraph& graph,
                                        const ModelDescriptor& descriptor,
                                        ValidationResult& out) {
  for (const auto& group : descriptor.selection_groups) {
    if (group.attach_to.size() < 2)
      continue;

    std::vector<glm::vec3> forwards;
    forwards.reserve(group.attach_to.size());

    for (const std::string& attach_id : group.attach_to) {
      auto it = graph.node_lookup.find(attach_id);
      if (it == graph.node_lookup.end())
        continue;

      // Transform engine-convention forward by the node's world matrix.
      glm::vec4 fwd =
          it->second->world_transform * glm::vec4(kLocalForward, 0.0f);
      glm::vec3 f(fwd);
      float len = glm::length(f);
      if (len < 1e-6f)
        continue;
      forwards.push_back(f / len);
    }

    if (forwards.size() < 2)
      continue;

    const glm::vec3& ref = forwards[0];
    for (size_t i = 1; i < forwards.size(); ++i) {
      float dot = glm::clamp(glm::dot(ref, forwards[i]), -1.0f, 1.0f);
      float angle = std::acos(dot);
      if (angle > kForwardAlignmentThresholdRad) {
        Diagnostic diag;
        diag.code = "group.forward_misaligned";
        diag.severity = Diagnostic::Severity::Warning;
        diag.group_id = group.group_id;
        diag.message = "Attachment forward axes diverge within group";
        out.diagnostics.push_back(std::move(diag));
        break;  // One warning per group is enough
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Check: compute geometric bounds
//   Fills per-part, per-group, and full-model AABBs. Never produces
//   diagnostics; pure data.
// ---------------------------------------------------------------------------
static void ComputeBounds(const ResolvedModel& resolved,
                          const ModelGraph& graph, ValidationResult& out) {
  for (const auto& d : resolved.descriptors) {
    AABB part_box;
    for (int mesh_idx : d.mesh_indices) {
      if (mesh_idx < 0 || mesh_idx >= static_cast<int>(graph.meshes.size()))
        continue;
      AABB mesh_box =
          ComputeMeshAABB(graph.meshes[mesh_idx], d.local_transform);
      MergeAABB(part_box, mesh_box);
    }

    out.per_part_bounds[d.descriptor_id] = part_box;
    MergeAABB(out.per_group_bounds[d.group_id], part_box);
    MergeAABB(out.model_bounds, part_box);
  }
}

// ---------------------------------------------------------------------------
// Populate raw selection/parameter data on the result.
// ---------------------------------------------------------------------------
static void RecordRawData(const ResolvedModel& resolved,
                          ValidationResult& out) {
  std::unordered_set<std::string> unique_parts;
  for (const auto& d : resolved.descriptors) {
    const std::string& id = d.descriptor_id;
    size_t at = id.find("_at_");
    unique_parts.insert(at == std::string::npos ? id : id.substr(0, at));

    ValidationResult::ResolvedEntry entry;
    entry.descriptor_id = d.descriptor_id;
    entry.group_id = d.group_id;
    entry.applied_rotation = d.applied_rotation;
    entry.applied_scale = d.applied_scale;
    entry.attach_to = d.attach_to;
    out.resolved_entries.push_back(std::move(entry));
  }

  out.selected_part_ids.assign(unique_parts.begin(), unique_parts.end());
  std::sort(out.selected_part_ids.begin(), out.selected_part_ids.end());
}

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------
ValidationResult ProcModelValidator::Validate(const ResolvedModel& resolved,
                                              const ModelGraph& graph,
                                              const ModelDescriptor& descriptor,
                                              int attempt_index) {

  ValidationResult out;
  out.model_id = resolved.model_id;
  out.seed = resolved.seed;
  out.attempt_index = attempt_index;
  out.timestamp_ms = NowMs();

  RecordRawData(resolved, out);

  CheckConstraints(resolved, descriptor, out);
  CheckAttachments(resolved, graph, out);
  CheckGroupActivation(resolved, descriptor, out);
  CheckForwardAxisConsistency(graph, descriptor, out);
  ComputeBounds(resolved, graph, out);

  // Overall pass = no Error-severity diagnostics
  out.passed = std::none_of(out.diagnostics.begin(), out.diagnostics.end(),
                            [](const Diagnostic& d) {
                              return d.severity == Diagnostic::Severity::Error;
                            });

  // Sort active_group_ids for deterministic output
  std::sort(out.active_group_ids.begin(), out.active_group_ids.end());
  out.active_group_ids.erase(
      std::unique(out.active_group_ids.begin(), out.active_group_ids.end()),
      out.active_group_ids.end());

  return out;
}

}  // namespace ProcModel