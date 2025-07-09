#ifndef CAMERA_CONTROLS_H
#define CAMERA_CONTROLS_H

#include <imgui.h>
#include "editor/runtime/application.h"
#include "game/game_test.h"

namespace Panels {

inline void CameraControls() {
  ImGui::BeginChild("CameraControls", ImVec2(300, 0), true);
  ImGui::Text("Camera Controls");
  ImGui::Separator();  // TODO: create custom UI separator

  auto* game = dynamic_cast<GameTest*>(Application::Game()->GetGame());
  if (game) {
    OrbitCamera& cam = game->camera;

    float pitch = cam.getPitch();
    float yaw = cam.getYaw();
    float roll = cam.getRoll();
    float dist = cam.getDistance();
    float target[3];
    std::memcpy(target, cam.getTarget(), sizeof(target));

    if (ImGui::SliderFloat("Pitch", &pitch, -bx::kPi, bx::kPi))
      cam.setPitch(pitch);
    if (ImGui::SliderFloat("Yaw", &yaw, -bx::kPi, bx::kPi))
      cam.setYaw(yaw);
    if (ImGui::SliderFloat("Roll", &roll, -bx::kPi, bx::kPi))
      cam.setRoll(roll);
    if (ImGui::SliderFloat("Distance", &dist, 1.0f, 1500.0f))
      cam.setDistance(dist);
    if (ImGui::SliderFloat3("Target", target, -2000.0f, 2000.0f))
      cam.setTarget(target);

    if (ImGui::Button("Reset Camera")) {
      cam.setDistance(100.0f);
      cam.setPitch(0.509f);
      cam.setYaw(1.422f);
      cam.setRoll(3.142f);
      float resetTarget[3] = {32.0f, -147.826f, 200.0f};
      cam.setTarget(resetTarget);
    }
  }

  ImGui::EndChild();
}

}  // namespace Panels

#endif  // CAMERA_CONTROLS_H
