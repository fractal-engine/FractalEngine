// ---------------------------------------------------------------------------
// scene_template.cpp
// Implements the default template scene logic used when a new scene is created
// in the editor.
// Purpose: Provides placeholder setup, update, and render behavior
// ---------------------------------------------------------------------------

#include "scene/scene_template.h"
#include "base/logger.h"

void SceneTemplate::Init() {
  Logger::getInstance().Log(LogLevel::Info, "[SceneTemplate] Init()");
  // Load test mesh, set up test light, etc.
}

void SceneTemplate::Update(float dt) {
  // Animate object? Rotate light? etc
}

void SceneTemplate::Render() {
  // Setup view/projection and draw a mesh
}
