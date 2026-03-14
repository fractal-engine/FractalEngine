#ifndef RUNTIME_H
#define RUNTIME_H

#include <memory>
#include <thread>

#include "editor/editor_base.h"
#include "editor/editor_ui.h"
#include "editor/pipelines/scene_view_pipeline.h"
#include "editor/project/project_manager.h"
#include "editor/state.h"

#include "engine/core/singleton.hpp"
#include "engine/renderer/frame_graph.h"
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

class ShadowMap;
class Model;

namespace Runtime {

// CORE
int START_LOOP();
int TERMINATE();

// Resource functions
void UpdateGlobalResources();
GlobalResources BuildGlobalResources();

// Project
ProjectManager& Project();

// Pipeline getters
SceneViewPipeline& GetSceneViewPipeline();  // ! rename it
FrameGraph& GetFrameGraph();                // ! rename it

// Getters
EditorBase* Editor();
RendererBase* Renderer();
GameManager* Game();
Input* InputDevice();
WindowManager* Window();
ShaderManager* Shader();
IMGizmo& SceneGizmos();
EditorState& State();

// Shadow getter
ShadowMap& MainShadowMap();

}  // namespace Runtime

#endif  // RUNTIME_H
