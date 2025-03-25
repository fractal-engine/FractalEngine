#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "base/game_base.h"

class GameManager {
  friend class SubsystemManager;
  enum GameState { ENDED, STARTING, RUNNING, ENDING, PAUSING, PAUSED };

private:
  std::unique_ptr<Game> game_;
  GameState gamestate_;
  std::thread game_thread_;

  uint64_t frame_count_;
  bool is_terminating_ = false;

  std::mutex state_mutex_;
  std::condition_variable condition_;

  GameManager() = delete;
  GameManager(std::unique_ptr<Game>&& game);

public:
  void StartGame();
  void EndGame();
  void Run();
  void Terminate();
  uint64_t GetFrameCount();
};

#endif  // GAME_MANAGER_H
