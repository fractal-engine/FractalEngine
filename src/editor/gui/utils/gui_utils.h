#ifndef GUI_UTILS_H
#define GUI_UTILS_H

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#include <algorithm>
#include <glm/glm.hpp>
#include <string>

// TODO: tie cursor logic with /platform/input/cursor

namespace GUIUtils {

// Lighten color by given amount
ImU32 Lighten(ImU32 color, float amount);

// Darken color by given amount
ImU32 Darken(ImU32 color, float amount);

// Lerp between given colors
ImVec4 LerpColors(const ImVec4& a, const ImVec4& b, float t);

// Return relative scroll value of current UI child (0.0 to 1.0)
float GetChildScrollValue();

// Calculate size and offset for aspect fitting
void CalculateAspectFitting(float aspectRatio, ImVec2& size, ImVec2& offset);

glm::vec2 KeepCursorInBounds(glm::vec4 bounds, bool& cursorMoved, float offset);

// Return title for window with adjusted spacing
std::string WindowTitle(const char* title);

// Return if the current window is focused
bool WindowFocused();

// Return if current window is hovered
bool WindowHovered();

}  // namespace GUIUtils

#endif  // GUI_UTILS_H