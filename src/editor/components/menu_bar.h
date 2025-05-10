#ifndef MENU_BAR_H
#define MENU_BAR_H

#include <imgui.h>

namespace Components {

inline void MenuBar(bool& quit, bool& debugHighlight, bool& showMetrics,
                    bool& showLog, bool& activatePicker) {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Exit", "Ctrl+Q"))
        quit = true;
      ImGui::EndMenu();
    }

    // ImGui debug menu
    if (ImGui::BeginMenu("Debug")) {
      ImGui::MenuItem("Highlight ID Conflicts", nullptr, &debugHighlight);
      ImGui::MenuItem("Show Metrics Window", nullptr, &showMetrics);
      ImGui::MenuItem("Show Debug Log Window", nullptr, &showLog);
      if (ImGui::MenuItem("Activate Picker"))
        activatePicker = true;
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
}

}  // namespace Components
#endif  // MENU_BAR_H
