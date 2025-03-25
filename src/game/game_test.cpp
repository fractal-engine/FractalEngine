#include "game/game_test.h"

#include "subsystem/subsystem_manager.h"

void GameTest::Update() {

  interval++;
  SubsystemManager::GetRenderer()->ClearDisplay();
  SubsystemManager::GetRenderer()->ShowText(asciiArt, pos_x, pos_y);

  if (SubsystemManager::GetInput()->IsJustPressed(Key::D)) {
    pos_x += 1;
  }
  if (SubsystemManager::GetInput()->IsJustPressed(Key::A)) {
    pos_x -= 1;
  }
  if (SubsystemManager::GetInput()->IsJustPressed(Key::W)) {
    pos_y -= 1;
  }
  if (SubsystemManager::GetInput()->IsJustPressed(Key::S)) {
    pos_y += 1;
  }
}