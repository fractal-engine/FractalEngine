#include "game_manager.h"
#include "engine/core/logger.h"

GameManager::GameManager(std::unique_ptr<GameBase>&& core)
    : gamestate_(GameState::ENDED),  // Start in the ENDED state.
      core_(std::move(core)),
      scene_manager_(std::make_unique<SceneManager>()),
      frame_count_(0) {}

void GameManager::Init() {
  if (core_) {
    core_->Init();
  }
}

// This is the new heart of the GameManager. It's called every frame from the
// main loop.
void GameManager::Update() {
  // Handle state transitions.
  if (gamestate_ == GameState::STARTING) {
    gamestate_ = GameState::RUNNING;
    Logger::getInstance().Log(LogLevel::Info, "Game manager state: RUNNING");
  } else if (gamestate_ == GameState::ENDING) {
    gamestate_ = GameState::ENDED;
    Logger::getInstance().Log(LogLevel::Info, "Game manager state: ENDED");
  }

  // Only update the core game logic if we are in the RUNNING state.
  if (gamestate_ == GameState::RUNNING && core_) {
    core_->Update();
    frame_count_++;
  }
}

// The renderer calls this every frame, regardless of state.
void GameManager::Render(const float* viewMatrix, const float* projMatrix) {
  if (core_) {
    core_->Render(viewMatrix, projMatrix);
  }
}

void GameManager::Destroy() {
  Logger::getInstance().Log(LogLevel::Info, "Game manager destroy initiated");
  if (core_) {
    core_->Destroy();
  }
}

// --- STATE CONTROL ---
// These are called by the UI signals. They just change the state.
void GameManager::StartGame() {
  Logger::getInstance().Log(LogLevel::Info, "Game manager state: STARTING");
  gamestate_ = GameState::STARTING;
}

void GameManager::EndGame() {
  Logger::getInstance().Log(LogLevel::Info, "Game manager state: ENDING");
  gamestate_ = GameState::ENDING;
}

void GameManager::Terminate() {
  // This function is now just a signal for cleanup.
  // The actual program termination is handled by the main loop in runtime.cpp.
}

uint64_t GameManager::GetFrameCount() const {
  return frame_count_;
}