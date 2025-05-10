#ifndef STATUS_BAR_H
#define STATUS_BAR_H

#include <imgui.h>
#include <imgui_internal.h>

namespace Components {

inline void StatusBar() {
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

  // handle positioning below MainMenuBar
  if (ImGui::BeginViewportSideBar("##SecondaryMenuBar", viewport, ImGuiDir_Up,
                                  ImGui::GetFrameHeight(), window_flags)) {
    if (ImGui::BeginMenuBar()) {
      ImGui::Text(
          "Project: [Placeholder] | Branch: [Placeholder] | Status: "
          "[Placeholder]");
      ImGui::EndMenuBar();
    }
  }
  ImGui::End();
}

}  // namespace Components

#endif  // STATUS_BAR_H
