// ---------------------------------------------------------------------------
// scene_manager.h
// Manages the current active scene in the engine.
// Responsible for loading, initializing, updating, and rendering scenes.
// ---------------------------------------------------------------------------

#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <memory>
#include "scene.h"

class SceneManager {
public:
  void LoadScene(std::unique_ptr<Scene> scene);
  void Update(float dt);
  void Render();

private:
  std::unique_ptr<Scene> current_scene_;
};

#endif  // SCENE_MANAGER_H