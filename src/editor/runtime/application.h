#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include "engine/core/singleton.hpp"

#include "editor/editor_base.h"
#include "editor/editor_layer.h"
#include "editor/project/project_manager.h"
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

namespace Application {
/* ── Lifecycle ───────────────────── */
static void Initialize();
static void Shutdown();

/* ---- Accessors ------------ */
EditorBase* Editor();
RendererBase* Renderer();
GameManager* Game();
Input* InputSystem();
WindowManager* Window();
ShaderManager* Shader();
ProjectManager& Project();
}  // namespace Application

#endif  // APPLICATION_H
