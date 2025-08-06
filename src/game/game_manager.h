// ---------------------------------------------------------------------------
// game_manager.h
// Oversees the main game loop and game state transitions.
// Owns core game logic and delegates scene control to SceneManager.
// TODO: Change name to GameLoopManager or something more appropriate.
// ---------------------------------------------------------------------------

#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <memory>
#include "engine/scene/scene_manager.h"
#include "game_base.h"

class GameManager {
private:
  std::unique_ptr<GameBase> core_;
  std::unique_ptr<SceneManager> scene_manager_;
  uint64_t frame_count_;
  bool is_game_running_ = false;  // The simple flag to track Play Mode

public:
  GameManager(std::unique_ptr<GameBase>&& core);

  // The new, simple, single-threaded interface
  void Init();     // Called once at startup by runtime.cpp
  void Update();   // Called once per frame by the main loop in runtime.cpp
  void Render(const float* viewMatrix,
              const float* projMatrix);  // Called once per frame
  void Destroy();  // Called once at shutdown

  // State control functions called by the UI signals
  void StartGame();
  void EndGame();

  uint64_t GetFrameCount() const;
  GameBase* GetGame() const { return core_.get(); }
};

#endif  // GAME_MANAGER_H