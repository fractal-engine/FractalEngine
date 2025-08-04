#include "game_manager.h"
#include "engine/core/logger.h"

GameManager::GameManager(std::unique_ptr<GameBase>&& core)
    : core_(std::move(core)),
      scene_manager_(std::make_unique<SceneManager>()),
      frame_count_(0) {}

void GameManager::Init() {
  if (core_) {
    core_->Init();
    Logger::getInstance().Log(LogLevel::Info, "Game initialized");
  }
}

void GameManager::Update() {
  // Only update the game logic if the game is in the "running" state.
  if (is_game_running_ && core_) {
    core_->Update();
    frame_count_++;
  }
  if (scene_manager_) {
    // scene_manager_->Update(1.0f / 60.0f); // we can update this too if
    // needed
  }
}

void GameManager::Render() {
  if (core_) {
    core_->Render();
  }
}

void GameManager::Destroy() {
  Logger::getInstance().Log(LogLevel::Info, "Game manager destroy initiated");
  if (core_) {
    core_->Destroy();
  }
}

// --- NEW STATE CONTROL IMPLEMENTATIONS ---

void GameManager::StartGame() {
  Logger::getInstance().Log(LogLevel::Info, "Game manager state: STARTING");
  is_game_running_ = true;
  // can also reset frame count or other logic here if needed.
}

void GameManager::EndGame() {
  Logger::getInstance().Log(LogLevel::Info, "Game manager state: ENDING");
  is_game_running_ = false;
}

uint64_t GameManager::GetFrameCount() const {
  return frame_count_;
}