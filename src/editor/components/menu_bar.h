#ifndef MENU_BAR_H
#define MENU_BAR_H

#include <imgui.h>
#include <imgui_internal.h>

namespace Components {

inline void MenuBar(bool& quit, bool& debugHighlight, bool& showMetrics,
                    bool& showLog, bool& activatePicker) {
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

  // handle positioning and sizing
  if (ImGui::BeginViewportSideBar("##MainMenuBar", viewport, ImGuiDir_Up,
                                  ImGui::GetFrameHeight(), window_flags)) {
    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Exit", "Ctrl+Q"))
          quit = true;
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Debug")) {
        ImGui::MenuItem("Highlight ID Conflicts", nullptr, &debugHighlight);
        ImGui::MenuItem("Show Metrics Window", nullptr, &showMetrics);
        ImGui::MenuItem("Show Debug Log Window", nullptr, &showLog);
        if (ImGui::MenuItem("Activate Picker"))
          activatePicker = true;
        ImGui::EndMenu();
      }

      ImGui::EndMenuBar();
    }
  }
  ImGui::End();
}

}  // namespace Components

#endif  // MENU_BAR_H
