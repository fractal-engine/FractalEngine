#ifndef ENTITY_CONTAINER_H
#define ENTITY_CONTAINER_H

#include <entt/entt.hpp>
#include <string>
#include <tuple>
#include <utility>

#include "engine/core/logger.h"
#include "engine/ecs/components/transform.h"

/*************************************************
-------------- SECURE ECS CONTAINER ----------------
**************************************************/
// TODO: implement explicit assignment operator logic for custom copy behavior
class EntityContainer {
public:
  // ctors
  EntityContainer() : handle_(entt::null), transform_(nullptr) {}
  EntityContainer(const EntityContainer& other) = default;

  explicit EntityContainer(Entity handle) : handle_(handle) {
    if (Verify())
      transform_ = &ECS::Main().Get<TransformComponent>(handle_);
  }

  EntityContainer(std::tuple<Entity, TransformComponent&> tup)
      : handle_(std::get<0>(tup)) {
    transform_ = &std::get<1>(tup);
  }

  EntityContainer& operator=(const EntityContainer&) = default;

  [[nodiscard]] Entity Handle() const { return handle_; }
  [[nodiscard]] bool Verify() const {
    return ECS::Main().Has<TransformComponent>(handle_);
  }

  [[nodiscard]] TransformComponent& Transform() {
    if (!transform_) {
      Warn("Could not fetch transform");
      static TransformComponent dummy;
      return dummy;
    }
    return *transform_;
  }

  [[nodiscard]] uint32_t Id() const { return transform_ ? transform_->id_ : 0; }
  [[nodiscard]] std::string Name() const {
    return transform_ ? transform_->name_ : "Invalid Entity";
  }

  template <typename T>
  bool Has() const {
    return ECS::Main().Has<T>(handle_);
  }
  template <typename T>
  bool CanAdd() const {
    return ECS::Main().CanAdd<T>(handle_);
  }

  template <typename T, typename... Args>
  T& Add(Args&&... args) {
    if (!Verify())
      return Fail<T>("add", "does not exist");
    if (!CanAdd<T>())
      return Fail<T>("add", "already owns it");
    return ECS::Main().Add<T>(handle_, std::forward<Args>(args)...);
  }

  template <typename T>
  T& Get() {
    if (!Verify())
      return Fail<T>("get", "does not exist");
    if (!Has<T>())
      return Fail<T>("get", "does not own it");
    return ECS::Main().Get<T>(handle_);
  }

  template <typename T>
  void Remove() {
    if (Has<T>())
      ECS::Main().Remove<T>(handle_);
  }

private:
  template <typename T>
  T& Fail(const std::string& op, const std::string& reason) const {
    Warn("'Could not " + op + " component because entity '" + Name() + "' " +
         reason);
    static T dummy{};
    return dummy;
  }

  static void Warn(const std::string& msg) {
    Logger::getInstance().Log(LogLevel::Warning,
                              std::string("EntityContainer: ") + msg);
  }

  // data
  Entity handle_;
  TransformComponent* transform_;
};

#endif  // ENTITY_CONTAINER_H
