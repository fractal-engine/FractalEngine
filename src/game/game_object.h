#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <string>

/*********************************************************************************
 * @class GameObject
 * @brief Basic game object with an ID and a name
 * @see GameObject.cpp
 *
 * This class serves as a minimal base for all in-game objects. It holds:
 *   - A unique ID (integer)
 *   - A name (string)
 *
 * Future Ideas:
 *   - Position/Rotation: Provide methods to track object transform.
 *   - Collision support: Enable collision detection and response.
 *   - Inheritance/Components: Extend via derived classes.
 *
 * Development:
 *   - Tied to GameObject.cpp, which contains the class implementation.
 *   - New methods or attributes should be declared here.
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

  // More attributes and methods here as needed

private:
  int id_;
  std::string name_;
};

#endif  // GAMEOBJECT_H
