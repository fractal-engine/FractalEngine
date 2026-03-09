#include "asset_graph_editor.h"
#include "editor/gui/styles/editor_styles.h"
#include <cmath>

AssetGraphEditor::AssetGraphEditor() {}

void AssetGraphEditor::Render() {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("Asset Graph", nullptr);

  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
  ImGui::BeginChild("GraphToolbar", ImVec2(0, 36.0f), true,
                    ImGuiWindowFlags_NoScrollbar);
  ImGui::Button(ICON_FA_PLUS " Add Node");
  ImGui::SameLine();
  ImGui::Button(ICON_FA_FLOPPY_DISK " Save Graph");
  ImGui::EndChild();
  ImGui::PopStyleVar();

  RenderNodeGraph(draw_list);

  ImGui::End();
  ImGui::PopStyleVar();
}

void AssetGraphEditor::RenderNodeGraph(ImDrawList* draw_list) {
  ImVec2 p_min = ImGui::GetCursorScreenPos();
  ImVec2 p_max = ImVec2(p_min.x + ImGui::GetContentRegionAvail().x,
                        p_min.y + ImGui::GetContentRegionAvail().y);

  draw_list->AddRectFilled(p_min, p_max, EditorColor::background);

  float grid_step = 64.0f;
  for (float x = std::fmod(p_min.x, grid_step); x < p_max.x - p_min.x;
       x += grid_step) {
    draw_list->AddLine(ImVec2(p_min.x + x, p_min.y),
                       ImVec2(p_min.x + x, p_max.y), EditorColor::border_color);
  }
  for (float y = std::fmod(p_min.y, grid_step); y < p_max.y - p_min.y;
       y += grid_step) {
    draw_list->AddLine(ImVec2(p_min.x, p_min.y + y),
                       ImVec2(p_max.x, p_min.y + y), EditorColor::border_color);
  }

  ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() * 0.5f - 100.0f,
                             ImGui::GetWindowHeight() * 0.5f));
  ImGui::TextDisabled("Procedural Node Graph Logic Here");
}