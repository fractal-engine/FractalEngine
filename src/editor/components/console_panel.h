#ifndef CONSOLE_PANEL_H
#define CONSOLE_PANEL_H

#include <imgui.h>
#include <string>
#include <vector>
#include "core/logger.h"

namespace Components {

inline void ConsolePanel() {
  ImGui::BeginChild("LogRegion", ImVec2(0, 0), true);
  ImGui::Text("Debug Log");
  ImGui::Separator();

  ImGui::BeginChild("LogScrollingRegion", ImVec2(0, 0), true,
                    ImGuiWindowFlags_HorizontalScrollbar);
  const auto& log_entries = Logger::getInstance().GetLogEntries();
  for (const auto& line : log_entries) {
    ImGui::TextUnformatted(line.c_str());
  }
  ImGui::EndChild();

  ImGui::EndChild();
}

}  // namespace Components

#endif  // CONSOLE_PANEL_H