#include "game_manager.h"
#include "engine/core/logger.h"

// The one and only constructor
GameManager::GameManager(std::unique_ptr<GameBase>&& core)
    : core_(std::move(core)),
      scene_manager_(std::make_unique<SceneManager>()),
      frame_count_(0) {}

void GameManager::Init() {
  // We call Init() here, once, when the GameManager is created.
  if (core_) {
    core_->Init();
    Logger::getInstance().Log(LogLevel::Info, "Game initialized");
  }
}

void GameManager::Update() {
  // The main loop in runtime.cpp calls this every frame.
  // We only run the game's core update logic if the state is "playing".
  if (is_game_running_ && core_) {
    core_->Update();
    frame_count_++;
  }
}

void GameManager::Render() {
  // The main loop calls this every frame.
  // We only render the game's content if the state is "playing".
  if (is_game_running_ && core_) {
    core_->Render();
  }
}

void GameManager::Destroy() {
  Logger::getInstance().Log(LogLevel::Info, "Game manager destroy initiated");
  if (core_) {
    core_->Destroy();
  }
}

// --- State Control Implementations ---

void GameManager::StartGame() {
  Logger::getInstance().Log(LogLevel::Info, "Game manager state: STARTING");
  is_game_running_ = true;
  // You could reset the frame count here if you want.
  // frame_count_ = 0;
}

void GameManager::EndGame() {
  Logger::getInstance().Log(LogLevel::Info, "Game manager state: ENDING");
  is_game_running_ = false;
}

uint64_t GameManager::GetFrameCount() const {
  return frame_count_;
}