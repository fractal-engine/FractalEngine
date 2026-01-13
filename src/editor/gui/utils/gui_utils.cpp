#include "gui_utils.h"

#include "editor/editor_ui.h"

#include "platform/platform_utils.h"
#include "platform/window_manager.h"

namespace GUIUtils {

ImU32 Lighten(ImU32 color, float amount) {
  float r = static_cast<float>((color >> 0) & 0xFF) / 255.0f;
  float g = static_cast<float>((color >> 8) & 0xFF) / 255.0f;
  float b = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
  int a = (color >> 24) & 0xFF;

  float factor = 1.0f + amount;
  r = std::min(r * factor, 1.0f);
  g = std::min(g * factor, 1.0f);
  b = std::min(b * factor, 1.0f);

  return IM_COL32(static_cast<int>(r * 255), static_cast<int>(g * 255),
                  static_cast<int>(b * 255), a);
}

ImU32 Darken(ImU32 color, float amount) {
  float r = static_cast<float>((color >> 0) & 0xFF) / 255.0f;
  float g = static_cast<float>((color >> 8) & 0xFF) / 255.0f;
  float b = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
  int a = (color >> 24) & 0xFF;

  float factor = 1.0f - amount;
  r = std::max(r * factor, 0.0f);
  g = std::max(g * factor, 0.0f);
  b = std::max(b * factor, 0.0f);

  return IM_COL32(static_cast<int>(r * 255), static_cast<int>(g * 255),
                  static_cast<int>(b * 255), a);
}

ImVec4 LerpColors(const ImVec4& a, const ImVec4& b, float t) {
  return ImVec4(a.x + t * (b.x - a.x), a.y + t * (b.y - a.y),
                a.z + t * (b.z - a.z), a.w + t * (b.w - a.w));
}

float GetChildScrollValue() {
  float scrollY = ImGui::GetScrollY();
  float maxScrollY = ImGui::GetScrollMaxY();
  if (maxScrollY <= 0.0f)
    return 0.0f;
  return scrollY / maxScrollY;
}

void CalculateAspectFitting(float aspectRatio, ImVec2& size, ImVec2& offset) {
  ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();

  // Scale by width first
  size.x = contentRegionAvail.x;
  size.y = contentRegionAvail.x / aspectRatio;

  // If height exceeds available, scale by height instead
  if (size.y > contentRegionAvail.y) {
    size.y = contentRegionAvail.y;
    size.x = contentRegionAvail.y * aspectRatio;
  }

  // Center offset
  offset = ImVec2((contentRegionAvail.x - size.x) * 0.5f,
                  (contentRegionAvail.y - size.y) * 0.5f);
}

glm::vec2 KeepCursorInBounds(glm::vec4 bounds, bool& cursorMoved,
                             float offset) {
  glm::vec2 currentPos = Platform::GetGlobalCursorPosition();
  glm::vec2 updatedPos = currentPos;
  cursorMoved = false;

  glm::vec2 min = glm::vec2(bounds.x, bounds.y);
  glm::vec2 max = glm::vec2(bounds.z, bounds.w);

  // Horizontal boundaries
  if (currentPos.x < min.x + offset) {
    updatedPos = glm::vec2(max.x - offset, currentPos.y);
    Platform::SetGlobalCursorPosition(updatedPos);
    cursorMoved = true;
  } else if (currentPos.x > max.x - offset) {
    updatedPos = glm::vec2(min.x + offset, currentPos.y);
    Platform::SetGlobalCursorPosition(updatedPos);
    cursorMoved = true;
  }

  // Vertical boundaries
  if (currentPos.y < min.y + offset) {
    updatedPos = glm::vec2(updatedPos.x, max.y - offset);
    Platform::SetGlobalCursorPosition(updatedPos);
    cursorMoved = true;
  } else if (currentPos.y > max.y - offset) {
    updatedPos = glm::vec2(updatedPos.x, min.y + offset);
    Platform::SetGlobalCursorPosition(updatedPos);
    cursorMoved = true;
  }

  return updatedPos;
}

std::string WindowTitle(const char* title) {
  return "     " + std::string(title) + "     ";
}

bool WindowFocused() {
  return ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
}

bool WindowHovered() {
  return ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
}

}  // namespace GUIUtils