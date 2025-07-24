#include "runtime.h"

#include "editor/registry/asset_registry.h"
#include "editor/registry/component_registry.h"
#include "engine/audio/sound_manager.h"  // TODO: remove later
#include "engine/context/engine_context.h"
#include "engine/core/logger.h"
#include "engine/renderer/icons/icon_loader.h"
#include "game/game_test.h"

// ------------------ single-instance state (internal linkage) -----------------
namespace Runtime {

// ---------- globals ----------
static std::thread g_game_thread;

std::unique_ptr<EditorBase> g_editor;
std::unique_ptr<GameManager> g_game_manager;

RendererBase* g_renderer = nullptr;
ShaderManager* g_shader_manager = nullptr;
Input* g_input = nullptr;
WindowManager* g_window_manager = nullptr;

// Project manager
ProjectManager g_project_manager;

// Pipelines
SceneViewPipeline g_scene_view_pipeline;

// TODO: scene gizmos

// TODO: default assets

// TODO: default settings

// TODO: global game state

static void _LoadDependencies() {
  // TODO: resource manager

  // TODO: load shaders
  // ShaderPool::loadAllSync("./shaders/materials");
  // ShaderPool::loadAllSync("./shaders/postprocessing");
  // ShaderPool::loadAllSync("./shaders/gizmo");
  // ShaderPool::loadAllSync("./shaders/passes");

  // TODO: create default textures

  // load icons
  IconLoader::CreatePlaceholderIcon("./resources/icons/fallback/fallback.png");
  IconLoader::LoadIcons("./resources/icons/shared");
  IconLoader::LoadIconsAsync("./resources/icons/assets");
  IconLoader::LoadIconsAsync("./resources/icons/components");
  IconLoader::LoadIconsAsync("./resources/icons/scene");

  // Create default skybox

  // Create default cubemap

  // Create default texture
}

static void _CreateResources() {

  // TODO: create pipelines here
  g_scene_view_pipeline.Create();

  // TODO: setup scene gizmos here

  // TODO: Create main shadow disk and main shadow map here ?
}

static void _CreateEngineContext() {
  if (!EngineContext::Init()) {
    Logger::getInstance().Log(LogLevel::Error, "EngineContext init failed");
    std::exit(1);
  }

  // cache subsystem references
  g_window_manager = &EngineContext::Window();
  g_renderer = &EngineContext::Renderer();
  g_shader_manager = &EngineContext::Shader();
  g_input = &EngineContext::Input();

  // Set audio
  // TODO: remove once we have a proper audio context
  SoundManager::Instance().setAmbientVolume(0.7f);
  SoundManager::Instance().startAmbient();
}

static void _LaunchEditor() {

  // TODO: add welcome console message here

  // Create Asset registry
  AssetRegistry::Create();

  // Create component registry
  // TODO: ComponentRegistry::Create();

  // initialize editor layer
  // TODO: should be EditorUI::Setup();
  g_editor = std::make_unique<EditorUI>(g_renderer);
  Logger::getInstance().Log(LogLevel::Info, "Editor initialized");

  // TODO: Show editor window here?
  // WindowContext::SetResizeable(true);
  // WindowContext::MaximizeWindow();
  // WindowContext::SetVisible(true);

  // Load example project
  std::filesystem::path project_path =
      std::filesystem::current_path() / "examples" / "example-project";
  std::filesystem::create_directories(project_path);
  g_project_manager.Load(project_path);

  // TODO: show welcome panel here
}

static void _NextFrame() {

  // TODO: Start EngineContext::_NextFrame(); ???

  // Render editor
  g_project_manager.PollEvents();
  // TODO: start profiler for ui
  // TODO: stop profiler for ui

  // TODO: Stop EngineContext::EndFrame(); ???
}

static void _ConnectSignals() {

  // connect editor event handles
  g_editor->game_start_pressed.connect([&] { g_game_manager->StartGame(); });
  g_editor->game_end_pressed.connect([&] { g_game_manager->EndGame(); });
  g_editor->editor_exit_pressed.connect([&] { g_game_manager->Terminate(); });

  g_editor->game_inputed.connect([&](InputEvent event) {
    g_input->FowardInputEvent(event, g_game_manager->GetFrameCount());
  });

  g_renderer->redrawn.connect([&] { g_editor->RequestUpdate(); });
}

int START_LOOP() {
  Logger::getInstance().Log(LogLevel::Info, "Runtime::StartLoop");
  _CreateEngineContext();
  _LoadDependencies();
  _CreateResources();
  _LaunchEditor();

  // Setup game (manager)
  g_game_manager = std::make_unique<GameManager>(std::make_unique<GameTest>());
  Logger::getInstance().Log(LogLevel::Info, "GameManager initialized");

  _ConnectSignals();

  // Setup game
  g_game_thread = std::thread([] {
    if (g_game_manager)
      g_game_manager->Run();
  });
  Logger::getInstance().Log(LogLevel::Info,
                            "Game initialized in its own thread");

  // Run the editor
  g_editor->Run();
  Logger::getInstance().Log(LogLevel::Info, "Editor initialized and running");

  // Main loop
  // TODO: uncomment once EditorUI::Run() is refactored
  /* while (EngineContext::Running())
    _NextFrame(); */

  // TODO: remove once _NextFrame() is used
  g_project_manager.PollEvents();

  return TERMINATE();
}

int TERMINATE() {

  // Terminate game thread
  if (g_game_manager) {
    g_game_manager->Terminate();
  }

  // join game thread
  if (g_game_thread.joinable())
    g_game_thread.join();

  // Stop game logic
  if (g_game_manager) {
    g_game_manager->Destroy();
    g_game_manager->Terminate();
    g_game_manager.reset();
  }

  // Destroy editor subsystems
  if (g_editor) {
    g_editor->Destroy();
    g_editor.reset();
  }

  // Stop pipelines
  g_scene_view_pipeline.Destroy();

  EngineContext::Destroy();

  Logger::getInstance().Log(LogLevel::Info, "Runtime::Terminate");

  // Exit application
  std::exit(0);
  return 0;
}

EditorBase* Editor() {
  return g_editor.get();
}
RendererBase* Renderer() {
  return g_renderer;
}
GameManager* Game() {
  return g_game_manager.get();
}
Input* InputSystem() {
  return g_input;
}
WindowManager* Window() {
  return g_window_manager;
}
ShaderManager* Shader() {
  return g_shader_manager;
}
ProjectManager& Project() {
  return g_project_manager;
}

SceneViewPipeline& sceneViewPipeline() {
  return g_scene_view_pipeline;
}

}  // namespace Runtime