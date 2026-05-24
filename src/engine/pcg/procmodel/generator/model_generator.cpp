#include "model_generator.h"

// #include <pcg_random.hpp>
#include <queue>
#include <random>

#include "engine/pcg/pipeline/linear_pipeline.h"
#include "engine/pcg/procmodel/generator/model_context.h"

#include "engine/core/logger.h"

namespace ProcModel {
namespace {

struct PendingActivation {
  const SelectionGroup* group;
  std::string activator_instance_id;
  std::string activator_part_id;
  glm::mat4 activator_world_transform;
};

}  // namespace

// Final matrix is composed in ModelInstantiator
static void ApplyParameterRanges(ResolvedDescriptor& resolved,
                                 const ModelGraphNode& node, pcg32& rng) {
  for (const auto* range : node.transform_ranges) {
    if (range->rotation_min && range->rotation_max) {
      std::uniform_real_distribution<float> dist_x(range->rotation_min->x,
                                                   range->rotation_max->x);
      std::uniform_real_distribution<float> dist_y(range->rotation_min->y,
                                                   range->rotation_max->y);
      std::uniform_real_distribution<float> dist_z(range->rotation_min->z,
                                                   range->rotation_max->z);

      resolved.applied_rotation +=
          glm::vec3(dist_x(rng), dist_y(rng), dist_z(rng));
    }

    if (range->scale_min && range->scale_max) {
      std::uniform_real_distribution<float> dist_x(range->scale_min->x,
                                                   range->scale_max->x);
      std::uniform_real_distribution<float> dist_y(range->scale_min->y,
                                                   range->scale_max->y);
      std::uniform_real_distribution<float> dist_z(range->scale_min->z,
                                                   range->scale_max->z);

      resolved.applied_scale *=
          glm::vec3(dist_x(rng), dist_y(rng), dist_z(rng));
    }
  }
}

static const PartDescriptor* WeightedSelect(
    const std::vector<const PartDescriptor*>& candidates, pcg32& rng) {
  float total_weight = 0.0f;
  for (const auto* c : candidates) {
    total_weight += c->weight;
  }

  std::uniform_real_distribution<float> dist(0.0f, total_weight);
  float roll = dist(rng);

  float cumulative = 0.0f;
  for (const auto* c : candidates) {
    cumulative += c->weight;
    if (roll <= cumulative) {
      return c;
    }
  }

  return candidates.back();
}

std::optional<InstanceData> ModelGenerator::Generate(
    const ModelGraph& graph, const ModelDescriptor& descriptor,
    const PCG::LinearPipeline& pipeline, uint64_t seed, int max_retries,
    ValidationLogger* validator_logger) {

  for (int attempt = 0; attempt < max_retries; ++attempt) {
    pcg32 rng(seed + attempt);

    std::unordered_set<std::string> selected_ids;
    std::vector<ResolvedDescriptor> resolved_descriptors;
    bool failed = false;

    // Collect root groups
    std::queue<PendingActivation> pending;
    for (const auto& group : descriptor.selection_groups) {
      if (group.parent.empty()) {
        pending.push({&group, "", "", glm::mat4(1.0f)});
      }
    }

    // --- diagnostic counters ---
    size_t groups_processed = 0;               // DEBUG
    size_t groups_enqueued = pending.size();   // DEBUG
    size_t max_pending_size = pending.size();  // DEBUG
    const size_t hard_guard =
        descriptor.selection_groups.size() * 32 + 128;  // DEBUG

    while (!pending.empty() && !failed) {

      // DEBUG
      if (groups_processed > hard_guard) {
        Logger::getInstance().Log(
            LogLevel::Error,
            "[Generator][Diag] Hard guard hit. Potential activation cycle. "
            "attempt=" +
                std::to_string(attempt) +
                " seed=" + std::to_string(seed + attempt) +
                " processed=" + std::to_string(groups_processed) +
                " enqueued=" + std::to_string(groups_enqueued) +
                " pending=" + std::to_string(pending.size()) +
                " maxPending=" + std::to_string(max_pending_size));
        failed = true;
        break;
      }  // DEBUG

      const PendingActivation pa = pending.front();
      pending.pop();
      ++groups_processed;  // DEBUG

      const SelectionGroup* group = pa.group;

      // Filter parts by constraints
      std::vector<const PartDescriptor*> valid;
      for (const auto& part : group->parts) {
        if (IsValidSelection(part.id, selected_ids, descriptor.constraints)) {
          valid.push_back(&part);
        }
      }

      if (valid.empty()) {
        if (group->required) {
          failed = true;
          break;
        }
        continue;
      }

      const PartDescriptor* chosen = WeightedSelect(valid, rng);
      selected_ids.insert(chosen->id);

      // Look up the graph node for mesh references
      auto it = graph.node_lookup.find(chosen->id);
      if (it == graph.node_lookup.end()) {
        failed = true;
        break;
      }

      ModelGraphNode* node = it->second;

      // Sockets live on the group, not the activator node
      const std::vector<std::string>* sockets_to_fill =
          group->sockets.has_value() ? &*group->sockets : nullptr;

      // Authored activator world — shared by both branches below.
      // We use this to extract each socket's *local* offset from the activator
      // as it sits in the GLTF (authored_inverse * socket->world_transform),
      // then re-apply that offset from the actual placed instance's world
      // transform (activator_world_transform * socket_local). This correctly
      // repositions sockets per-instance without baking any authored position.
      glm::mat4 authored_activator_world(1.0f);
      if (!pa.activator_part_id.empty()) {
        auto act_it = graph.node_lookup.find(pa.activator_part_id);
        if (act_it != graph.node_lookup.end()) {
          authored_activator_world = act_it->second->world_transform;
        }
      }
      glm::mat4 authored_inverse = glm::inverse(authored_activator_world);

      std::vector<ResolvedDescriptor> newly_added;

      // Variant-only groups fire once per activating instance: each gets a
      // unique descriptor_id via '_under_<activator>' suffix.
      // local_transform is rebased from the activator instance's world
      // transform, so children on instanced parents are correctly positioned
      // per-parent rather than at the GLTF-authored position.
      if (!sockets_to_fill || sockets_to_fill->empty()) {
        // Variant-only: re-base onto activator instance world transform
        glm::mat4 local_offset = authored_inverse * node->world_transform;
        glm::mat4 instance_world = pa.activator_world_transform * local_offset;

        ResolvedDescriptor resolved;
        resolved.descriptor_id = chosen->id;
        resolved.group_id = group->group_id;
        resolved.mesh_indices = node->mesh_indices;
        resolved.local_transform = instance_world;
        resolved.activator_id = pa.activator_instance_id;

        // Sample parameter ranges from annotations
        ApplyParameterRanges(resolved, *node, rng);

        newly_added.push_back(std::move(resolved));
      } else {
        // Socket-based: one instance per socket, world-positioned per instance
        const PartDescriptor* uniform_pick =
            !group->select_per_socket ? chosen : nullptr;

        // Parent scale — match by activator instance id
        glm::vec3 parent_scale(1.0f);
        if (!pa.activator_instance_id.empty()) {
          for (const auto& prev : resolved_descriptors) {
            if (prev.descriptor_id == pa.activator_instance_id) {
              parent_scale = prev.applied_scale;
              break;
            }
          }
        }

        for (const auto& socket_id : *sockets_to_fill) {
          auto sock_it = graph.node_lookup.find(socket_id);
          if (sock_it == graph.node_lookup.end())
            continue;

          const PartDescriptor* per_attach = group->select_per_socket
                                                 ? WeightedSelect(valid, rng)
                                                 : uniform_pick;

          auto graph_it = graph.node_lookup.find(per_attach->id);
          if (graph_it == graph.node_lookup.end())
            continue;

          ModelGraphNode* per_socket_node = graph_it->second;

          // socket_local: offset of this socket from its authored activator
          // socket_world: same offset re-applied from the instance position
          glm::mat4 socket_local =
              authored_inverse * sock_it->second->world_transform;
          glm::mat4 socket_world = pa.activator_world_transform * socket_local;

          // DEBUG
          glm::vec3 sw(socket_world[3]);
          Logger::getInstance().Log(
              LogLevel::Debug, "[Generator] socket=" + socket_id +
                                   " world_xz=(" + std::to_string(sw.x) + ", " +
                                   std::to_string(sw.z) + ")");

          // DEBUG
          glm::vec3 fwd_world = glm::vec3(socket_world * glm::vec4(0, 0, 1, 0));
          Logger::getInstance().Log(
              LogLevel::Debug, "[Generator] socket=" + socket_id + " pos=(" +
                                   std::to_string(sw.x) + "," +
                                   std::to_string(sw.z) + ")" + " fwd=(" +
                                   std::to_string(fwd_world.x) + "," +
                                   std::to_string(fwd_world.z) + ")");

          ResolvedDescriptor attached;
          attached.descriptor_id =
              std::string(per_attach->id) + "_at_" + socket_id;
          if (!pa.activator_instance_id.empty()) {
            attached.descriptor_id += "_under_" + pa.activator_instance_id;
          }
          attached.group_id = group->group_id;
          attached.mesh_indices = per_socket_node->mesh_indices;
          attached.local_transform = socket_world;
          attached.activator_id = pa.activator_instance_id;

          // Sample scale factor with optional jitter
          float factor = group->scale_factor;
          if (group->scale_jitter) {
            std::uniform_real_distribution<float> jdist(-*group->scale_jitter,
                                                        *group->scale_jitter);
            factor += jdist(rng);
          }
          attached.applied_scale = parent_scale * factor;

          // Sample parameter ranges for THIS attachment, so each attached
          // copy gets independent rotation/scale jitter.
          ApplyParameterRanges(attached, *per_socket_node, rng);

          // Per-attachment rotation jitter: each attached instance picks an
          // independent rotation perturbation, so identical-mesh
          // attachments stick out in different directions.
          const glm::vec3& j = group->rotation_jitter;
          if (j.x > 0.0f || j.y > 0.0f || j.z > 0.0f) {
            std::uniform_real_distribution<float> dx(-j.x, j.x);
            std::uniform_real_distribution<float> dy(-j.y, j.y);
            std::uniform_real_distribution<float> dz(-j.z, j.z);
            attached.applied_rotation += glm::vec3(dx(rng), dy(rng), dz(rng));
          }

          // Track the picked part for constraint validation (only when
          // varying)
          if (group->select_per_socket)
            selected_ids.insert(per_attach->id);

          newly_added.push_back(std::move(attached));
        }
      }
      // Activate child groups PER PLACED INSTANCE
      for (const auto& new_desc : newly_added) {
        for (const auto& g : descriptor.selection_groups) {
          for (const auto& parent_id : g.parent) {
            if (parent_id == chosen->id) {
              pending.push({&g, new_desc.descriptor_id, chosen->id,
                            new_desc.local_transform});
              ++groups_enqueued;
              max_pending_size = std::max(max_pending_size, pending.size());
              break;
            }
          }
        }
      }
      for (auto& nd : newly_added) {
        resolved_descriptors.push_back(std::move(nd));
      }
    }

    Logger::getInstance().Log(
        LogLevel::Debug,
        "[Generator][Diag] attempt=" + std::to_string(attempt) +
            " seed=" + std::to_string(seed + attempt) +
            " failed=" + std::to_string(failed ? 1 : 0) +
            " processed=" + std::to_string(groups_processed) +
            " enqueued=" + std::to_string(groups_enqueued) +
            " maxPending=" + std::to_string(max_pending_size) +
            " selected=" + std::to_string(selected_ids.size()) +
            " resolved=" + std::to_string(resolved_descriptors.size()));

    if (failed)
      continue;

    // Apply parameter bindings across resolved descriptors
    ApplyParameterBindings(resolved_descriptors, descriptor.parameter_bindings);

    InstanceModel result;
    result.model_id = descriptor.model_id;
    result.seed = seed + attempt;
    result.descriptors = std::move(resolved_descriptors);

    // TODO: revise the if condition here
    if (descriptor.scale_min && descriptor.scale_max) {
      std::uniform_real_distribution<float> dx(descriptor.scale_min->x,
                                               descriptor.scale_max->x);
      std::uniform_real_distribution<float> dy(descriptor.scale_min->y,
                                               descriptor.scale_max->y);
      std::uniform_real_distribution<float> dz(descriptor.scale_min->z,
                                               descriptor.scale_max->z);
      result.model_scale = glm::vec3(dx(rng), dy(rng), dz(rng));
    } else {
      result.model_scale = glm::vec3(1.0f);
    }

    // Run pipeline operations on resolved model
    {
      ModelContext ctx(descriptor, graph, result, rng);
      pipeline.Run(ctx);

      // Final constraint check
      if (ValidateConstraints(selected_ids, descriptor.constraints)) {
        // Post-generation validation
        if (validator_logger) {
          ProcModelSample vr =
              ProcModelValidator::Validate(result, graph, descriptor, attempt);
          validator_logger->Write(vr);
          Logger::getInstance().Log(
              LogLevel::Debug,
              "[Generator] Validation: passed=" + std::to_string(vr.passed) +
                  " diagnostics=" + std::to_string(vr.diagnostics.size()));
          if (!vr.passed) {
            continue;  // Retry with next seed
          }
        }

        InstanceData out;
        out.model = std::move(result);
        out.instance_geometry = std::move(ctx.instance_geometry);

        Logger::getInstance().Log(
            LogLevel::Debug,
            "[Generator] Returning InstanceData: descriptors=" +
                std::to_string(out.model.descriptors.size()) +
                " geometry_count=" +
                std::to_string(out.instance_geometry.size()));

        return out;
      }
    }
  }

  return std::nullopt;
}

bool ModelGenerator::IsValidSelection(
    const std::string& part_id,
    const std::unordered_set<std::string>& selected_ids,
    const std::vector<ConstraintRule>& constraints) {
  for (const auto& rule : constraints) {
    if (rule.type == ConstraintRule::Type::EXCLUDES) {
      // If part_a is already selected, part_b is invalid (and vice versa)
      if (rule.part_a == part_id && selected_ids.count(rule.part_b) > 0)
        return false;
      if (rule.part_b == part_id && selected_ids.count(rule.part_a) > 0)
        return false;
    }
    if (rule.type == ConstraintRule::Type::REQUIRES) {
      // If this part requires another that hasn't been selected yet,
      // skip — it may be selected later. Only reject if the required
      // part was already excluded by group processing.
      // For now, REQUIRES is checked in ValidateConstraints
      // post-generation.
    }
  }
  return true;
}

void ModelGenerator::ApplyParameterBindings(
    std::vector<ResolvedDescriptor>& descriptors,
    const std::vector<ParameterBinding>& bindings) {
  std::unordered_map<std::string, ResolvedDescriptor*> lookup;
  for (auto& desc : descriptors) {
    lookup[desc.descriptor_id] = &desc;
  }

  for (const auto& binding : bindings) {
    auto src_it = lookup.find(binding.source_part);
    auto tgt_it = lookup.find(binding.target_part);

    if (src_it == lookup.end() || tgt_it == lookup.end())
      continue;

    if (binding.source_param == "rotation" &&
        binding.target_param == "rotation") {
      tgt_it->second->applied_rotation =
          src_it->second->applied_rotation * binding.ratio;
    }
  }
}

bool ModelGenerator::ValidateConstraints(
    const std::unordered_set<std::string>& selected_ids,
    const std::vector<ConstraintRule>& constraints) {
  for (const auto& rule : constraints) {
    bool a_selected = selected_ids.count(rule.part_a) > 0;
    bool b_selected = selected_ids.count(rule.part_b) > 0;

    switch (rule.type) {
      case ConstraintRule::Type::EXCLUDES:
        if (a_selected && b_selected)
          return false;
        break;
      case ConstraintRule::Type::REQUIRES:
        if (a_selected && !b_selected)
          return false;
        if (b_selected && !a_selected)
          return false;
        break;
    }
  }
  return true;
}

}  // namespace ProcModel
