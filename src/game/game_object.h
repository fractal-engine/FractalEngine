#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <bgfx/bgfx.h>  // For bgfx handles
#include <glm/glm.hpp>  // For transform matrix
#include <string>

/*********************************************************************************
 * @class GameObject
 * @brief Basic game object with an ID and a name
 * @see GameObject.cpp
 *
 * This class serves as a minimal base for all in-game objects. It holds:
 *   - A unique ID (integer)
 *   - A name (string)
 **********************************************************************************/

class GameObject {
public:
  // Constructor
  GameObject(int id, const std::string& name);

  // Virtual destructor
  virtual ~GameObject();

  // Accessors
  int GetId() const;
  std::string GetName() const;

private:
  int id_;
  std::string name_;
};

#endif  // GAMEOBJECT_H
