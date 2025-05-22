#include "application.h"
#include "engine/core/logger.h"
#include "game/game_test.h"
#include "game/game_manager.h"
#include "engine/renderer/renderer_graphics.h"
#include "platform/window_manager.h"

using namespace std;

void Application::Initialize() {
  getInstance().initialize();
}

// Function to get the editor instance
const unique_ptr<EditorBase>& Application::GetEditor() {
  return getInstance().editor_;
}

// Function to get the renderer instance
const std::unique_ptr<RendererBase>& Application::GetRenderer() {
  return getInstance().renderer_;
}

// Function to get the game manager instance
const std::unique_ptr<GameManager>& Application::GetGameManager() {
  return getInstance().game_manager_;
}

const std::unique_ptr<Input>& Application::GetInput() {
  return getInstance().input_;
}

const std::unique_ptr<WindowManager>& Application::GetWindowManager() {
  return getInstance().window_manager_;
}

const std::unique_ptr<ShaderManager>& Application::GetShaderManager() {
  return getInstance().shader_manager_;
}

// FIXME: fix constructors/destructors to use .reset only
// Some functions might be more appropriate to use in window_manager
void Application::initialize() {
  // 1. Initialize window manager
  window_manager_ = std::make_unique<WindowManager>();
  if (!window_manager_->Initialize("Fractal Engine", 1280, 720)) {
    Logger::getInstance().Log(LogLevel::Error,
                              "WindowManager failed to initialize.");
    std::exit(1);
  }

  // 2. Initialize renderer + bgfx
  renderer_ = std::make_unique<GraphicsRenderer>();
  if (!static_cast<GraphicsRenderer*>(renderer_.get())->InitBgfx()) {
    Logger::getInstance().Log(LogLevel::Error,
                              "BGFX failed to initialize in renderer!");
    std::exit(1);
  }

  Logger::getInstance().Log(LogLevel::Info, "Renderer initialized");

  // 3. Initialize ShaderManager
  shader_manager_ = std::make_unique<ShaderManager>();
  shader_manager_->Init();
  Logger::getInstance().Log(LogLevel::Info, "Shader Manager initialized");

  // 4. Initialize Input
  input_ = std::make_unique<Input>();
  Logger::getInstance().Log(LogLevel::Info, "Input initialized");

  // 5. Initialize Editor
  editor_.reset(new EditorLayer(renderer_));
  Logger::getInstance().Log(LogLevel::Info, "Editor initialized");

  // TODO - Read Game Manager read games from filesystem
  // 6. Initialize GameManager
  game_manager_.reset(new GameManager(std::make_unique<GameTest>()));
  Logger::getInstance().Log(LogLevel::Info, "Game Manager initialized");

  // Connect editor signals to game manager
  editor_->game_start_pressed.connect([&] { game_manager_->StartGame(); });
  editor_->game_end_pressed.connect([&] { game_manager_->EndGame(); });
  editor_->editor_exit_pressed.connect([&] { game_manager_->Terminate(); });
  editor_->game_inputed.connect([&](InputEvent event) {
    input_->FowardInputEvent(event, game_manager_->GetFrameCount());
  });
  renderer_->redrawn.connect([&] { editor_->RequestUpdate(); });

  game_manager_->StartGame();  // Start the game
}

void Application::Shutdown() {
  Logger::getInstance().Log(LogLevel::Info,
                            "=== Application::Shutdown called ===");

  // 1. Stop the game thread
  if (getInstance().game_manager_) {
    getInstance().game_manager_->Shutdown();   // destroy BGFX resources
    getInstance().game_manager_->Terminate();  // stop thread
  }

  // 2. Shutdown ImGui and Editor
  if (getInstance().editor_) {
    Logger::getInstance().Log(LogLevel::Info, "Shutting down Editor");
    getInstance().editor_->Shutdown();
  }

  // 3. Shutdown ShaderManager
  if (getInstance().shader_manager_) {
    Logger::getInstance().Log(LogLevel::Info, "Shutting down Shader Manager");
    getInstance().shader_manager_->Shutdown();
  }

  // 4. Shutdown graphics renderer
  if (getInstance().renderer_) {
    Logger::getInstance().Log(LogLevel::Info, "Shutting down Renderer");
    getInstance().renderer_->Shutdown();
  }

  // 5. reset all subsystems
  getInstance().game_manager_.reset();
  getInstance().input_.reset();
  getInstance().editor_.reset();
  getInstance().renderer_.reset();
  getInstance().window_manager_.reset();
  getInstance().shader_manager_.reset();

  Logger::getInstance().Log(LogLevel::Info,
                            "=== Application::Shutdown complete ===");
}
