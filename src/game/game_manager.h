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
  bool is_game_running_ = false;  // To track play/stop state

public:
  GameManager(std::unique_ptr<GameBase>&& core);

  // --- THE NEW, SIMPLE INTERFACE ---
  void Init();     // Called once at startup.
  void Update();   // Called once per frame by the main loop.
  void Render();   // Called once per frame by the main loop.
  void Destroy();  // Called once at shutdown.

  // --- STATE CONTROL (called by UI signals) ---
  void StartGame();  // This now just sets a flag
  void EndGame();    // This also just sets a flag

  uint64_t GetFrameCount() const;
  GameBase* GetGame() const { return core_.get(); }
};

#endif  // GAME_MANAGER_H