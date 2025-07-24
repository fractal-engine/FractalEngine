#ifndef RUNTIME_H
#define RUNTIME_H

#include <memory>
#include <thread>

#include "editor/editor_base.h"
#include "editor/editor_ui.h"
#include "editor/pipelines/scene_view_pipeline.h"
#include "editor/project/project_manager.h"
#include "engine/core/singleton.hpp"
#include "engine/renderer/renderer_base.h"
#include "engine/renderer/shaders/shader_manager.h"
#include "game/game_manager.h"
#include "platform/input/input.h"
#include "platform/window_manager.h"

// Forward declarations
class EditorBase;
class GameManager;
class RendererBase;
class ShaderManager;
class Input;
class WindowManager;
class ProjectManager;

namespace Runtime {

/* ------------ Core functions ------------ */
int START_LOOP();
int TERMINATE();

/* ------------ Project ------------ */
ProjectManager& Project();

/* ------------ Pipeline getters ------------ */
SceneViewPipeline& sceneViewPipeline();

/* ------------ Getters ------------ */
EditorBase* Editor();
RendererBase* Renderer();
GameManager* Game();
Input* InputSystem();
WindowManager* Window();
ShaderManager* Shader();

}  // namespace Runtime

#endif  // RUNTIME_H
