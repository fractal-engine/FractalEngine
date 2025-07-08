#ifndef CONSOLE_PANEL_H
#define CONSOLE_PANEL_H

#include <imgui.h>
#include <string>
#include <vector>
#include "engine/core/logger.h"
#include "editor/resources/theme/dark_theme.hpp"

namespace Panels {

inline void ConsolePanel() {
  ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
  ImGui::BeginChild("LogRegion", ImVec2(0, 0), true);
  ImGui::BeginChild("LogScrollingRegion", ImVec2(0, 0), false,
                    ImGuiWindowFlags_HorizontalScrollbar);

  ImGui::PushFont(Theme::console_font);  // Set console font

  // Display log
  const auto& log_entries = Logger::getInstance().GetLogEntries();
  for (const auto& line : log_entries) {
    ImGui::TextUnformatted(line.c_str());
  }

  ImGui::PopFont();  // Reset to default font

  ImGui::EndChild();
  ImGui::EndChild();
  ImGui::PopStyleVar();
}

}  // namespace Panels

#endif  // CONSOLE_PANEL_H