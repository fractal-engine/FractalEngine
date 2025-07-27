#ifndef GAMEOBJECTMANAGER_H
#define GAMEOBJECTMANAGER_H

#include <memory>         //memory management and smart pointers
#include <unordered_map>  //hash table to store all game objects

#include "engine/core/singleton.hpp"  // Include singleton base class
#include "game_object.h"

class GameObjectManager : public Singleton<GameObjectManager> {
public:
  void AddGameObject(
      std::shared_ptr<GameObject> obj) {  // Add a new game object
    gameObjects_[obj->GetId()] = obj;
  }

  void RemoveGameObject(int id) {
    gameObjects_.erase(id);
  }  // remove game object

  std::shared_ptr<GameObject> GetGameObject(int id) {
    auto it = gameObjects_.find(id);
    return it != gameObjects_.end() ? it->second : nullptr;
  }

  const std::unordered_map<int, std::shared_ptr<GameObject>>&
  GetAllGameObjects() const {
    return gameObjects_;
  }

private:
  GameObjectManager() = default;
  friend class Singleton<GameObjectManager>;

  std::unordered_map<int, std::shared_ptr<GameObject>> gameObjects_;
};

#endif  // GAMEOBJECTMANAGER_H
