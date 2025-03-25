#include "subsystem/subsystem_manager.h"

#include "base/logger.h"
#include "editor/editor_tui.h"
#include "game/game_test.h"
#include "subsystem/renderer_text.h"

using namespace std;

void SubsystemManager::Initialize() {
  getInstance().initialize();
}

// Function to get the editor instance
const unique_ptr<EditorBase>& SubsystemManager::GetEditor() {
  return getInstance().editor_;
}

// Function to get the renderer instance
const std::unique_ptr<RendererBase>& SubsystemManager::GetRenderer() {
  return getInstance().renderer_;
}

// Function to get the game manager instance
const std::unique_ptr<GameManager>& SubsystemManager::GetGameManager() {
  return getInstance().game_manager_;
}

const std::unique_ptr<Input>& SubsystemManager::GetInput() {
  return getInstance().input_;
}
void SubsystemManager::initialize() {
  // STUB - Wrap this to a new function
  renderer_.reset(new Renderer());
  Logger::getInstance().Log(LogLevel::INFO, "Renderer initialized");
  editor_.reset(new Editor(renderer_));
  Logger::getInstance().Log(LogLevel::INFO, "Editor initialized");
  input_.reset(new Input());

  // STUB - For testing, directly used GameTest
  // TODO - Read Game Manager read games from filesystem
  game_manager_.reset(new GameManager(std::make_unique<GameTest>()));
  // Create GameManager using new

  editor_->game_start_pressed.connect([&] { game_manager_->StartGame(); });
  editor_->game_end_pressed.connect([&] { game_manager_->EndGame(); });
  editor_->editor_exit_pressed.connect([&] { game_manager_->Terminate(); });
  editor_->game_inputed.connect([&](InputEvent event) {
    input_->FowardInputEvent(event, game_manager_->GetFrameCount());
  });
  renderer_->redrawn.connect([&] { editor_->RequestUpdate(); });
}