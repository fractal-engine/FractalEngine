#ifndef MENU_BAR_H
#define MENU_BAR_H

#include <imgui.h>

namespace Components {

inline void MenuBar(bool& quit, bool& showMetrics, bool& showLog) {
  if (!ImGui::BeginMenuBar())
    return;

  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("Exit", "Ctrl+Q")) {
      quit = true;
    }
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Debug")) {
    ImGui::MenuItem("Show Metrics Window", nullptr, &showMetrics);
    ImGui::MenuItem("Show Debug Log Window", nullptr, &showLog);
    if (ImGui::MenuItem("Activate Picker"))
      ImGui::DebugStartItemPicker();
    ImGui::EndMenu();
  }

  ImGui::EndMenuBar();
}

}  // namespace Components
#endif  // MENU_BAR_H