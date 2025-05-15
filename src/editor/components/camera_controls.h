#ifndef CAMERA_CONTROLS_H
#define CAMERA_CONTROLS_H

#include <imgui.h>
#include "game/game_test.h"
#include "subsystem/subsystem_manager.h"

namespace Components {

inline void CameraControls() {
  ImGui::BeginChild("CameraControls", ImVec2(300, 0), true);
  ImGui::Text("Camera Controls");
  ImGui::Separator(); //TODO: create custom UI separator 

  auto* game =
      dynamic_cast<GameTest*>(SubsystemManager::GetGameManager()->GetGame());
  if (game) {
    ImGui::SliderFloat3("Eye Position", game->cameraEye, -200.0f, 200.0f);
    ImGui::SliderFloat3("Look At", game->cameraAt, -200.0f, 200.0f);
    ImGui::SliderFloat3("Up Vector", game->cameraUp, -1.0f, 1.0f);
    ImGui::SliderFloat("FOV", &game->cameraFOV, 10.0f, 120.0f);

    if (ImGui::Button("Reset Camera")) {
      game->cameraEye[0] = 120.0f;
      game->cameraEye[1] = 60.0f;
      game->cameraEye[2] = 32.0f;
      game->cameraAt[0] = 32.0f;
      game->cameraAt[1] = 0.0f;
      game->cameraAt[2] = 32.0f;
      game->cameraUp[0] = 1.0f;
      game->cameraUp[1] = 0.0f;
      game->cameraUp[2] = 0.0f;
      game->cameraFOV = 80.0f;
    }
  }

  ImGui::EndChild();
}

}  // namespace Components

#endif  // CAMERA_CONTROLS_H