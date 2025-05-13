#ifndef MENU_BAR_H
#define MENU_BAR_H

#include <imgui.h>
#include <imgui_internal.h>

namespace Components {

void MenuBar(std::function<void()> onExit, bool& debug_highlight,
             bool& show_metrics, bool& show_log, bool& activate_picker) {
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  float height = ImGui::GetFrameHeight();
  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

  if (ImGui::BeginViewportSideBar("##MainMenuBar", viewport, ImGuiDir_Up,
                                  height, window_flags)) {
    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Exit") && onExit)
          onExit();
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Debug")) {
        ImGui::MenuItem("Highlight ID Conflicts", nullptr, &debug_highlight);
        ImGui::MenuItem("Show Metrics Window", nullptr, &show_metrics);
        ImGui::MenuItem("Show Debug Log Window", nullptr, &show_log);
        if (ImGui::MenuItem("Activate Picker"))
          activate_picker = true;
        ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
    }
    ImGui::End();
  }
}

}  // namespace Components

#endif  // MENU_BAR_H
