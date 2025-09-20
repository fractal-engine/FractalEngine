#include "world.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "engine/core/logger.h"
#include "engine/renderer/model/model.h"

/* ───────── singleton ───────── */
ECS& ECS::Main() {
  if (!entt::locator<ECS>::has_value())
    Logger::getInstance().Log(LogLevel::Error,
                              "Fatal: No main ECS service was instantiated");

  return entt::locator<ECS>::value();
}

/* ───────── constructor ───────── */
ECS::ECS() : registry_(), id_counter_(0), render_queue_() {
  // Register mesh renderer events
  registry_.on_construct<MeshRendererComponent>()
      .connect<&ECS::InsertMeshRenderer>(this);
  registry_.on_destroy<MeshRendererComponent>()
      .connect<&ECS::PurgeMeshRenderer>(this);
}

uint32_t ECS::GetId() {
  return ++id_counter_;
}

/* ---------- render queue ---------- */
void ECS::InsertMeshRenderer(Entity target) {
  // gather
  std::vector<Entity> targets;
  View<MeshRendererComponent>().each(
      [&](auto e, const auto&) { targets.push_back(e); });

  // sort by shader-id then material-id
  // TODO: Use once Shader and Material are implemented
  /* std::sort(targets.begin(), targets.end(), [&](auto lhs, auto rhs) {
    auto& lmr = Get<MeshRendererComponent>(lhs);
    auto& rmr = Get<MeshRendererComponent>(rhs);
    auto ls = lmr.material ? lmr.material->GetShaderId() : -1;
    auto rs = rmr.material ? rmr.material->GetShaderId() : -1;
    if (ls != rs)
      return ls < rs;
    auto lm = lmr.material ? lmr.material->GetId() : -1;
    auto rm = rmr.material ? rmr.material->GetId() : -1;
    return lm < rm;
  });*/

  // rebuild queue
  render_queue_.clear();
  for (auto e : targets)
    render_queue_.emplace_back(e, Get<TransformComponent>(e),
                               Get<MeshRendererComponent>(e));
}

void ECS::PurgeMeshRenderer(Entity target) {
  std::vector<Entity> targets;
  View<MeshRendererComponent>().each([&](auto e, const auto&) {
    if (e != target)
      targets.push_back(e);
  });

  // TODO: Use once Shader and Material are implemented
  /* std::sort(targets.begin(), targets.end(), [&](auto lhs, auto rhs) {
    auto& lmr = Get<MeshRendererComponent>(lhs);
    auto& rmr = Get<MeshRendererComponent>(rhs);
    auto ls = lmr.material ? lmr.material->GetShaderId() : -1;
    auto rs = rmr.material ? rmr.material->GetShaderId() : -1;
    if (ls != rs)
      return ls < rs;
    auto lm = lmr.material ? lmr.material->GetId() : -1;
    auto rm = rmr.material ? rmr.material->GetId() : -1;
    return lm < rm;
  });*/

  render_queue_.clear();
  for (auto e : targets)
    render_queue_.emplace_back(e, Get<TransformComponent>(e),
                               Get<MeshRendererComponent>(e));
}

/* ---------- entity creation ---------- */
std::tuple<Entity, TransformComponent&> ECS::CreateEntity(
    const std::string& name) {
  Entity entity = registry_.create();
  TransformComponent& transform = Add<TransformComponent>(entity);

  transform.id_ = GetId();
  transform.name_ = name;
  transform.modified_ = true;

  return std::tuple<Entity, TransformComponent&>(entity, transform);
}

std::tuple<Entity, TransformComponent&> ECS::CreateEntity(
    const std::string& name, Entity parent) {
  if (!Has<TransformComponent>(parent)) {
    Logger::getInstance().Log(
        LogLevel::Warning,
        "[ECS]: Tried to create entity '" + name + "' with invalid parent");
    return CreateEntity(name);
  }

  auto [entity, transform] = CreateEntity(name);
  SetParent(entity, parent);
  return std::tuple<Entity, TransformComponent&>(entity, transform);
}

/* ---------- hierarchy ---------- */
void ECS::SetParent(Entity entity, Entity parent) {
  if (!Has<TransformComponent>(entity)) {
    Logger::getInstance().Log(
        LogLevel::Warning, "[ECS]: Tried to modify parent of invalid entity");
    return;
  }
  if (!Has<TransformComponent>(parent)) {
    Logger::getInstance().Log(LogLevel::Warning,
                              "[ECS]: Tried to make invalid entity a parent");
    return;
  }

  RemoveParent(entity);

  TransformComponent& transform = Get<TransformComponent>(entity);
  TransformComponent& parent_transform = Get<TransformComponent>(parent);

  parent_transform.children_.push_back(entity);
  transform.parent_ = parent;
  transform.depth_ = parent_transform.depth_ + 1;
  transform.modified_ = true;
}

void ECS::RemoveParent(Entity entity) {
  TransformComponent& transform = Get<TransformComponent>(entity);

  // TODO: replace with Transform implementation
  // Directly check parent_ member
  if (transform.parent_ == entt::null)
    return;

  // Get parent's children_ vector
  TransformComponent& parent = Get<TransformComponent>(transform.parent_);
  auto& children = parent.children_;
  auto it = std::find(children.begin(), children.end(), entity);

  if (it != children.end()) {
    std::swap(*it, children.back());
    children.pop_back();
  }

  // Example:
  /* if (!Transform::HasParent(transform))
    return;

  auto& children = Transform::GetParent(transform).children_;
  auto it = std::find(children.begin(), children.end(), entity);

  if (it != children.end()) {
    std::swap(*it, children.back());
    children.pop_back();
  }*/

  transform.parent_ = entt::null;
  transform.depth_ = 0;
  transform.modified_ = true;
}

/* ---------- camera query ---------- */
std::optional<Camera> ECS::GetActiveCamera() {
  auto group = registry_.group<TransformComponent>(entt::get<CameraComponent>);
  for (auto entity : group) {
    auto [transform, camera] =
        group.get<TransformComponent, CameraComponent>(entity);
    if (camera.enabled_)
      return Camera(transform, camera);
  }
  return std::nullopt;
}

/* ---------- per-frame transform evaluation ---------- */
// TODO: check if we need to directly use TransformSystem here
/* void ECS::UpdateTransforms() {
  registry_.view<TransformComponent>().each([](auto& t) {
    if (!t.modified_)
      return;
    t.model_ = glm::translate(glm::mat4(1.0f), t.position_) *
               glm::mat4_cast(t.rotation_) *
               glm::scale(glm::mat4(1.0f), t.scale_);
    t.normal_ = glm::transpose(glm::inverse(glm::mat3(t.model_)));
    t.modified_ = false;
  });
} */
