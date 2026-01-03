#ifndef CONSOLE_PANEL_H
#define CONSOLE_PANEL_H

#include <imgui.h>
#include <string>
#include <vector>

#include "editor/gui/styles/editor_styles.h"
#include "engine/core/logger.h"

namespace Panels {

inline void ConsolePanel() {

  // Set background color and corner radius
  ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(25, 26, 28, 255));
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);

  ImGui::BeginChild("LogRegion", ImVec2(0, 0), true);
  ImGui::BeginChild("LogScrollingRegion", ImVec2(0, 0), false,
                    ImGuiWindowFlags_HorizontalScrollbar);

  ImGui::PushFont(EditorStyles::GetFonts().console);  // Set console font

  // Display log
  const auto& log_entries = Logger::getInstance().GetLogEntries();
  for (const auto& line : log_entries) {
    ImGui::TextUnformatted(line.c_str());
  }

  // Pop after rendering
  ImGui::PopFont();
  ImGui::EndChild();
  ImGui::EndChild();

  ImGui::PopStyleColor();
  ImGui::PopStyleVar(2);
}

}  // namespace Panels

#endif  // CONSOLE_PANEL_H