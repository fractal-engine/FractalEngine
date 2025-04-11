// ---------------------------------------------------------------------------
// game_manager.h
// Oversees the main game loop and game state transitions.
// Owns core game logic and delegates scene control to SceneManager.
// TODO: Change name to GameLoopManager or something more appropriate.
// ---------------------------------------------------------------------------

#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "base/game_base.h"

#include "scene/scene_manager.h"

class GameManager {
  friend class SubsystemManager;
  enum GameState { ENDED, STARTING, RUNNING, ENDING, PAUSING, PAUSED };

private:
  std::unique_ptr<GameBase> core_;
  std::unique_ptr<SceneManager> scene_manager_;

  GameState gamestate_;
  std::thread game_thread_;
  uint64_t frame_count_;
  bool is_terminating_ = false;

  std::mutex state_mutex_;
  std::condition_variable condition_;

  GameManager() = delete;
  GameManager(std::unique_ptr<GameBase>&& core);
  void LoadScene(std::unique_ptr<Scene> scene);

  bool is_running_ = false;

public:
  void Render();
  void StartGame();
  void EndGame();
  void Run();
  void Terminate();
  void Shutdown();
  uint64_t GetFrameCount();
};

#endif  // GAME_MANAGER_H
