#include "transform_system.h"

#include <algorithm>
#include <unordered_set>

#include "engine/ecs/ecs_collection.h"
#include "engine/transform/transform.h"

// TODO: include diagnostics/profiler here

void TransformSystem::Perform(glm::mat4 /*viewProjection*/) {
  // Transform pass: compute model/normal only. MVP is done later in the render
  // stage.
  auto& ecs = ECS::Main();
  auto view = ecs.View<TransformComponent>();

  // 1) Collect unique roots of dirty subtrees (robust even if parents are
  // broken).
  std::vector<Entity> roots;
  roots.reserve(64);
  std::unordered_set<entt::entity> seen;

  for (auto e : view) {
    auto& t = view.get<TransformComponent>(e);
    if (!t.modified_)
      continue;

    // climb to the topmost ancestor with a valid transform parent chain
    Entity cur = e;
    while (true) {
      auto& ct = ecs.Get<TransformComponent>(cur);
      const bool has_valid_parent =
          (ct.parent_ != entt::null) && ecs.Has<TransformComponent>(ct.parent_);
      if (!has_valid_parent)
        break;
      cur = ct.parent_;
    }
    if (seen.insert(cur).second)
      roots.push_back(cur);
  }

  // Optional fallback: if nothing is dirty, we’re done.
  if (roots.empty())
    return;

  // 2) Iterative DFS from each dirty root; propagate "dirty" down.
  struct Node {
    Entity e;
    bool parent_dirty;
  };
  std::vector<Node> stack;
  stack.reserve(roots.size() * 4);
  for (auto r : roots)
    stack.push_back({r, true});  // root is treated as dirty

  while (!stack.empty()) {
    auto [e, parent_dirty] = stack.back();
    stack.pop_back();
    auto& t = ecs.Get<TransformComponent>(e);

    const bool dirty = parent_dirty || t.modified_;
    if (dirty) {
      if (t.parent_ != entt::null && ecs.Has<TransformComponent>(t.parent_)) {
        auto& p = ecs.Get<TransformComponent>(t.parent_);
        Transform::Evaluate(t, p);  // world = parent.world * local
      } else {
        Transform::Evaluate(t);  // world = local
      }
    }

    for (Entity c : t.children_) {
      if (ecs.Has<TransformComponent>(c))
        stack.push_back({c, dirty});
    }

    t.modified_ = false;  // clear after processing
  }
}

/* void TransformSystem::Evaluate(TransformComponent& transform) {
  if (transform.modified_) {
    Transform::Evaluate(transform);
  }
  for (Entity child : transform.children_) {
    auto& ecs = ECS::Main();
    auto& childTransform = ecs.Get<TransformComponent>(child);
    Evaluate(childTransform, transform, transform.modified_);
  }
  transform.modified_ = false;
}

void TransformSystem::Evaluate(TransformComponent& transform,
                              TransformComponent& parent,
                               bool propagateModified) {
  if (propagateModified)
    transform.modified_ = true;
  if (transform.modified_) {
    Transform::Evaluate(transform, parent);
  }
  for (Entity child : transform.children_) {
    auto& ecs = ECS::Main();
    auto& childTransform = ecs.Get<TransformComponent>(child);
    Evaluate(childTransform, transform, transform.modified_);
  }
  transform.modified_ = false;
}*/