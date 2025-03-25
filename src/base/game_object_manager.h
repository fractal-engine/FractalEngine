#ifndef GAMEOBJECTMANAGER_H
#define GAMEOBJECTMANAGER_H

#include <memory>         //memory management and smart pointers
#include <unordered_map>  //hash table to store all game objects

#include "game_object.h"
#include "singleton.hpp"  // Include singleton base class

class GameObjectManager : public Singleton<GameObjectManager> {
public:
  void AddGameObject(int id, const std::string& name) {  // add new game object
    gameObjects_[id] = std::make_shared<GameObject>(id, name);
  }

  void RemoveGameObject(int id) {
    gameObjects_.erase(id);
  }  // remove game object

  std::unique_ptr<GameObject> GetGameObject(int id) {
    auto it = gameObjects_.find(id);
    return it != gameObjects_.end()
               ? it->second
               : nullptr;  // if game object doesn't exist, return null (is
                           // there an alternate to null pointer?)
  }

private:
  GameObjectManager() = default;
  friend class Singleton<GameObjectManager>;

  std::unordered_map<int, std::unique_ptr<GameObject>> gameObjects_;
};

#endif  // GAMEOBJECTMANAGER_H
