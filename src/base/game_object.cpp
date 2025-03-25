#include "base/game_object.h"

/*********************************************************************************
 * @brief Implementation of GameObject class
 * @see GameObject.h
 *
 * Provides:
 *   - Constructor for assigning ID and Name
 *   - virtual destructor that currently does nothing
 *   - Basic getters for ID and Name
 *
 * Development:
 *   - Tied to GameObject.h, which contains the class definition.
 *   - New methods or attributes should be implemented here, follow
 *GameObject.h.
 **********************************************************************************/

// Constructor definition
GameObject::GameObject(int id, const std::string& name)
    : id_(id), name_(name) {}

// Destructor definition
GameObject::~GameObject() {}

int GameObject::GetId() const {
  return id_;
}

std::string GameObject::GetName() const {
  return name_;
}