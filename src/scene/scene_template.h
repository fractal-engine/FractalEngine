// ---------------------------------------------------------------------------
// scene_template.h
// Default template scene implementation used when users create a new scene.
// Purpose: Provides a minimal scene setup with placeholder logic, acts as a
// "scaffold" for users to use.
// ---------------------------------------------------------------------------

#ifndef SCENE_TEMPLATE_H
#define SCENE_TEMPLATE_H

#include "scene/scene.h"

class SceneTemplate : public Scene {
public:
  void Init() override;
  void Update(float dt) override;
  void Render() override;

private:
  // Store camera, objects, lights, etc
};

#endif  // SCENE_TEMPLATE_H