#include "subsystem/game_manager.h"

#include <chrono>
#include <thread>

#include "base/game_base.h"
#include "base/logger.h"

// GameManager::GameManager() : gamestate_(GameState::ENDED) {}

GameManager::GameManager(std::unique_ptr<Game>&& game)
    : gamestate_(GameState::ENDED), game_(std::move(game)), frame_count_(0) {
  Logger::getInstance().Log(LogLevel::INFO, "Game Manager initialized");
}

void GameManager::StartGame() {
  Logger::getInstance().Log(LogLevel::INFO, "Game manager start game");
  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    gamestate_ = GameState::STARTING;
  }
  condition_.notify_one();
}

void GameManager::EndGame() {
  Logger::getInstance().Log(LogLevel::INFO, "Game manager end game");
  std::lock_guard<std::mutex> lock(state_mutex_);
  gamestate_ = GameState::ENDING;
}

void GameManager::Terminate() {
  Logger::getInstance().Log(LogLevel::INFO, "Game manager terminating");
  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    is_terminating_ = true;
  }
  condition_.notify_one();
}

void GameManager::Run() {
  Logger::getInstance().Log(LogLevel::INFO, "Game thread started");

  while (true) {
    if (gamestate_ != RUNNING && !is_terminating_) {
      std::unique_lock<std::mutex> lock(state_mutex_);

      if (gamestate_ == GameState::STARTING) {
        // game_->start();
        gamestate_ = GameState::RUNNING;
      } else if (gamestate_ == GameState::ENDING) {
        // game_->end();
        Logger::getInstance().Log(LogLevel::DEBUG, "Game thread ending to end");
        gamestate_ = ENDED;
      } else if (gamestate_ == GameState::PAUSING) {
        gamestate_ = PAUSED;
      }
      if (!is_terminating_ && gamestate_ == GameState::PAUSED ||
          gamestate_ == GameState::ENDED) {
        Logger::getInstance().Log(LogLevel::DEBUG, "Game thread Sleep");
        condition_.wait(lock, [this] {
          return is_terminating_ || (gamestate_ != GameState::ENDED &&
                                     gamestate_ != GameState::PAUSED);
        });
      }
    }

    if (is_terminating_) {
      Logger::getInstance().Log(LogLevel::INFO,
                                "Game thread return on terminating");
      return;
    }
    game_->Update();
    frame_count_++;
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
}

uint64_t GameManager::GetFrameCount() {
  return frame_count_;
}
