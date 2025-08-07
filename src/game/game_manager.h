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
public:  // Public for easy state checking by the UI
  enum GameState { ENDED, STARTING, RUNNING, ENDING, PAUSING, PAUSED };

private:
  std::unique_ptr<GameBase> core_;
  std::unique_ptr<SceneManager> scene_manager_;

  GameState gamestate_;  // The state machine is back.
  uint64_t frame_count_;

public:
  GameManager(std::unique_ptr<GameBase>&& core);

  // --- THE NEW, SINGLE-THREADED INTERFACE ---
  void Init();
  void Update();  // This will be our "tick" function, called by the main loop.
  void Render(const float* viewMatrix,
              const float* projMatrix);  // Accepts camera matrices.
  void Destroy();

  // State control functions, now just simple state setters.
  void StartGame();
  void EndGame();
  void Terminate();  // This is just for cleanup now.

  uint64_t GetFrameCount() const;
  GameBase* GetGame() const { return core_.get(); }
  GameState GetState() const { return gamestate_; }
};

#endif  // GAME_MANAGER_H