#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

/* Compatibility for ImGui::GetKeyIndex (imgui-node-editor)
 * Required as imgui-node-editor uses an older version of ImGui where
 * TODO: Remove this file once issue is resolved
 */
namespace ImGui {
inline ImGuiKey GetKeyIndex(ImGuiKey key) {
  return key;
}
}  // namespace ImGui