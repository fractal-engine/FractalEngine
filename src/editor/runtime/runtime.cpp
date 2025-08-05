#include "runtime.h"

#include "editor/registry/asset_registry.h"
#include "editor/registry/component_registry.h"
#include "engine/audio/sound_manager.h"  // TODO: remove later
#include "engine/context/engine_context.h"
#include "engine/core/logger.h"
#include "engine/ecs/ecs_collection.h"
#include "engine/renderer/icons/icon_loader.h"
#include "game/game_test.h"

#include "imgui.h" 
#include <SDL.h>

// ------------------ single-instance state (internal linkage) -----------------
namespace Runtime {

// ---------- globals ----------

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
  // 1. Process asset events from the file watcher. This is where imports
  // happen.
  g_project_manager.PollEvents();

  // 2. Update the game state. This is now on the main thread.
  if (g_game_manager) {
    g_game_manager->Update();
  }

  // 3. Render the 3D scene into its framebuffer.
  g_scene_view_pipeline.Render();
}

static void _ConnectSignals() {

  // connect editor event handles
  g_editor->game_start_pressed.connect([&] { g_game_manager->StartGame(); });
  g_editor->game_end_pressed.connect([&] { g_game_manager->EndGame(); });
  g_editor->editor_exit_pressed.connect([] { TERMINATE(); });

  g_editor->game_inputed.connect([&](InputEvent event) {
    g_input->FowardInputEvent(event, g_game_manager->GetFrameCount());
  });
}

int START_LOOP() {
  Logger::getInstance().Log(LogLevel::Info, "Runtime::StartLoop");
  _CreateEngineContext();
  _LoadDependencies();
  _LaunchEditor();
  _CreateResources();
  

  // Initialize the game manager. It no longer starts its own thread.
  g_game_manager = std::make_unique<GameManager>(std::make_unique<GameTest>());
  g_game_manager->Init();  // Call one-time setup
  Logger::getInstance().Log(LogLevel::Info, "GameManager initialized");

  _ConnectSignals();

  // --- THIS IS THE NEW, UNIFIED MAIN LOOP ---
  while (EngineContext::Running()) {
    // A. Poll platform events (mouse, keyboard, window)
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      static_cast<EditorUI*>(g_editor.get())->HandleEvent(event); // Handle other input here or pass it to the editor
      if (event.type == SDL_QUIT) {
        // This needs to be connected to quit logic
        // For now, we can call TERMINATE directly.
        return TERMINATE();
      }
    }
    // B. Prepare the frame for rendering
    bgfx::setViewClear(ViewID::UI_BACKGROUND, BGFX_CLEAR_COLOR, 0x1e1e1eff,
                       1.0f, 0);
    bgfx::touch(ViewID::UI_BACKGROUND);

    // C. Prepare the ImGui frame
    static_cast<EditorUI*>(g_editor.get())->BeginFrame();

    // D. Run all core logic (game update, scene render)
    _NextFrame();

    // E. Render the Editor UI panels themselves
    static_cast<EditorUI*>(g_editor.get())->RenderPanels();

    // F. Finalize and submit the entire frame
    // Instead of calling the backend function directly, we call our new
    // encapsulated method.
    static_cast<EditorUI*>(g_editor.get())->RenderDraws();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    bgfx::frame();
  }

  return TERMINATE();
}

int TERMINATE() {

  // Stop game logic
  if (g_game_manager) {
    g_game_manager->Destroy();
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