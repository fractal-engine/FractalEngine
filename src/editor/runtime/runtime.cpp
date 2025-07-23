#include "runtime.h"

#include "engine/core/logger.h"
#include "engine/renderer/icons/icon_loader.h"
#include "engine/runtime/engine_context.h"
#include "game/game_test.h"

// ------------------ single-instance state (internal linkage) -----------------
namespace {
std::unique_ptr<EditorBase> g_editor;
std::unique_ptr<GameManager> g_game_manager;

RendererBase* g_renderer = nullptr;
ShaderManager* g_shader_manager = nullptr;
Input* g_input = nullptr;
WindowManager* g_window_manager = nullptr;

ProjectManager g_project_manager;
}  // namespace

// -----------------------------------------------------------------------------
//  System initialization and lifecycle
// -----------------------------------------------------------------------------
namespace Runtime {

static void InitialiseInternal();
static void ShutdownInternal();

void Initialize() {
  InitialiseInternal();
}
void Shutdown() {
  ShutdownInternal();
}

/* ---- accessors --------------------------- */
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

// ───────────────────────────────────────────
//  boot sequence (should run once)
//  TODO: change friend constructors to static
//  Create/factory functions
// ───────────────────────────────────────────
static void InitialiseInternal() {
  Logger::getInstance().Log(LogLevel::Info, "Runtime::Initialize");

  // initialize engine runtime subsystems
  if (!EngineContext::Init()) {
    Logger::getInstance().Log(LogLevel::Error, "runtime::Init() failed");
    std::exit(1);
  }

  // cache subsystem references
  g_window_manager = &EngineContext::Window();
  g_renderer = &EngineContext::Renderer();
  g_shader_manager = &EngineContext::Shader();
  g_input = &EngineContext::Input();

  // load dependencies

  // load icons
  IconLoader::CreatePlaceholderIcon("./resources/icons/fallback/fallback.png");
  IconLoader::LoadIcons("./resources/icons/shared");
  IconLoader::LoadIconsAsync("./resources/icons/assets");
  IconLoader::LoadIconsAsync("./resources/icons/components");
  IconLoader::LoadIconsAsync("./resources/icons/scene");

  // Create default skybox

  // Create default cubemap

  // Create default texture

  // Load example project
  std::filesystem::path project_path =
      std::filesystem::current_path() / "examples" / "example-project";
  std::filesystem::create_directories(project_path);
  g_project_manager.Load(project_path);

  // initialize editor layer
  g_editor = std::make_unique<EditorUI>(g_renderer);
  Logger::getInstance().Log(LogLevel::Info, "Editor initialized");

  // initialize game manager
  g_game_manager = std::make_unique<GameManager>(std::make_unique<GameTest>());
  Logger::getInstance().Log(LogLevel::Info, "GameManager initialized");

  // connect editor event handles
  g_editor->game_start_pressed.connect([&] { g_game_manager->StartGame(); });
  g_editor->game_end_pressed.connect([&] { g_game_manager->EndGame(); });
  g_editor->editor_exit_pressed.connect([&] { g_game_manager->Terminate(); });

  g_editor->game_inputed.connect([&](InputEvent event) {
    g_input->FowardInputEvent(event, g_game_manager->GetFrameCount());
  });

  g_renderer->redrawn.connect([&] { g_editor->RequestUpdate(); });
}

static void ShutdownInternal() {
  Logger::getInstance().Log(LogLevel::Info, "Runtime::Shutdown");

  // stop game logic
  if (g_game_manager) {
    g_game_manager->Shutdown();
    g_game_manager->Terminate();
    g_game_manager.reset();
  }

  // destroy editor subsystems
  if (g_editor) {
    g_editor->Shutdown();
    g_editor.reset();
  }

  EngineContext::Shutdown();
}
}  // namespace Runtime
