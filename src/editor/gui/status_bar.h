#ifndef STATUS_BAR_H
#define STATUS_BAR_H

#include <imgui.h>
#include <imgui_internal.h>

namespace Panels {

void StatusBar() {
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  float height = ImGui::GetFrameHeight();
  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove;

  if (ImGui::BeginViewportSideBar("##SecondaryMenuBar", viewport, ImGuiDir_Down,
                                  height, window_flags)) {
    if (ImGui::BeginMenuBar()) {
      ImGui::Text(
          "Project: [Placeholder] | Branch: [Placeholder] | Status: "
          "[Placeholder]");
      ImGui::EndMenuBar();
    }
    ImGui::End();
  }
}

}  // namespace Panels

#endif  // STATUS_BAR_H
