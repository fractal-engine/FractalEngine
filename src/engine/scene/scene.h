// ---------------------------------------------------------------------------
// scene.h
// Abstract base class for all scenes.
// Purpose: Defines the standard interface for lifecycle methods like Init,
// Update, and Render. Concrete scenes should inherit from this class.
// Defines what a scene is and what it should do.
// ---------------------------------------------------------------------------

#ifndef SCENE_H
#define SCENE_H

#include <memory>
#include <vector>

class Renderable;
// class Light;

class Scene {
public:
  virtual ~Scene() = default;
  virtual void Init() = 0;
  virtual void Update(float dt) = 0;
  virtual void Render() = 0;
};

#endif  // SCENE_H