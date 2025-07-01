#include "application.h"
#include "engine/core/logger.h"
#include "engine/renderer/icons/icon_loader.h"
#include "engine/runtime/core_loop.h"
#include "game/game_test.h"

// ───────────────────────────────────────────
//  system initialization and lifecycle
// ───────────────────────────────────────────
void Application::Initialize() {
  getInstance().InitializeInternal();
}

void Application::Shutdown() {
  Logger::getInstance().Log(LogLevel::Info, "Application::Shutdown");

  auto& self = getInstance();  // shorthand

  /* 1 – stop game logic */
  if (self.game_manager_) {
    self.game_manager_->Shutdown();
    self.game_manager_->Terminate();
    self.game_manager_.reset();
  }

  /* 2 – destroy editor subsystems */
  if (self.editor_) {
    self.editor_->Shutdown();
    self.editor_.reset();
  }

  /* 3 – shutdown engine runtime */
  runtime::Shutdown();
}

// ───────────────────────────────────────────
//  getters
// ───────────────────────────────────────────
EditorBase* Application::GetEditor() {
  return getInstance().editor_.get();
}
RendererBase* Application::GetRenderer() {
  return getInstance().renderer_;
}
GameManager* Application::GetGameManager() {
  return getInstance().game_manager_.get();
}
Input* Application::GetInput() {
  return getInstance().input_;
}
WindowManager* Application::GetWindowManager() {
  return getInstance().window_manager_;
}
ShaderManager* Application::GetShaderManager() {
  return getInstance().shader_manager_;
}

// ───────────────────────────────────────────
//  boot sequence (should run once)
//  TODO: change friend constructors to static
//  Create/factory functions
// ───────────────────────────────────────────
void Application::InitializeInternal() {
  Logger::getInstance().Log(LogLevel::Info, "Application::Initialize");

  /* 1 – initialize engine runtime subsystems */
  if (!runtime::Init()) {
    Logger::getInstance().Log(LogLevel::Error, "runtime::Init() failed");
    std::exit(1);
  }

  /* 2 – cache subsystem references */
  window_manager_ = &runtime::Window();
  renderer_ = &runtime::Renderer();
  shader_manager_ = &runtime::Shader();
  input_ = &runtime::Input();

  /* load dependencies */

  // load icons
  IconLoader::CreatePlaceholderIcon(
      "./resources/icons/fallback/fallback_icon.png");
  IconLoader::LoadIcons("./resources/icons/shared");
  IconLoader::LoadIconsAsync("./resources/icons/assets");
  IconLoader::LoadIconsAsync("./resources/icons/components");
  IconLoader::LoadIconsAsync("./resources/icons/scene");

  // Create default skybox

  // Create default cubemap

  // Create default texture

  /* 3 – initialize editor layer */
  editor_ = std::make_unique<EditorLayer>(renderer_);
  Logger::getInstance().Log(LogLevel::Info, "Editor initialized");

  /* 4 – initialize game manager */
  game_manager_ = std::unique_ptr<GameManager>(
      new GameManager(std::make_unique<GameTest>()));
  Logger::getInstance().Log(LogLevel::Info, "GameManager initialized");

  /* 5 – connect editor event handles */
  editor_->game_start_pressed.connect([&] { game_manager_->StartGame(); });
  editor_->game_end_pressed.connect([&] { game_manager_->EndGame(); });
  editor_->editor_exit_pressed.connect([&] { game_manager_->Terminate(); });

  editor_->game_inputed.connect([&](InputEvent event) {
    input_->FowardInputEvent(event, game_manager_->GetFrameCount());
  });

  renderer_->redrawn.connect([&] { editor_->RequestUpdate(); });
}
