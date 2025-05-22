#pragma once

#include_next "imgui.h"     
#include "imgui_internal.h"     

#undef RenderArrow
#undef TreeNodeEx
#undef CollapsingHeader

#define RenderArrow      RenderArrowRight
#define TreeNodeEx       TreeNodeExRight
#define CollapsingHeader CollapsingHeaderRight

namespace ImGui
{
    // Right-aligned arrow replacement
    IMGUI_API void RenderArrowRight(ImDrawList* draw_list, ImVec2 pos, ImU32 col, ImGuiDir dir, float scale = 1.0f);

    // TreeNode and CollapsingHeader shims
    IMGUI_API bool TreeNodeExRight(const char* label, ImGuiTreeNodeFlags flags = 0);
    IMGUI_API bool CollapsingHeaderRight(const char* label, ImGuiTreeNodeFlags flags = 0, bool* p_open = nullptr);
} // namespace ImGui
