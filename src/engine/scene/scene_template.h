// ---------------------------------------------------------------------------
// scene_template.h
// Default template scene implementation used when users create a new scene.
// Purpose: Provides a minimal scene setup with placeholder logic, acts as a
// "scaffold" for users to use.
// ---------------------------------------------------------------------------

#ifndef SCENE_TEMPLATE_H
#define SCENE_TEMPLATE_H

#include "scene.h"

class SceneTemplate : public Scene {
public:
  void Create() override;
  void Update(float dt) override;

private:
  // Store camera, objects, lights, etc
};

#endif  // SCENE_TEMPLATE_H