// ---------------------------------------------------------------------------
// scene_manager.cpp
// Implements logic for managing scene lifecycles, including switching and
// delegation.
// ---------------------------------------------------------------------------

#include "scene_manager.h"

void SceneManager::LoadScene(std::unique_ptr<Scene> scene) {
  current_scene_ = std::move(scene);
  if (current_scene_) {
    current_scene_->Init();
  }
}

void SceneManager::Update(float dt) {
  if (current_scene_)
    current_scene_->Update(dt);
}

void SceneManager::Render() {
  if (current_scene_)
    current_scene_->Render();
}
