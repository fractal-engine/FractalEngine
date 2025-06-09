/* #ifndef DROP_SHADOWS_H
#define DROP_SHADOWS_H

#include <imgui.h>
#include <imgui_internal.h>

namespace ui::shadow {
// this is called once per frame after ImGui::NewFrame()
inline void BeginFrame(bool enable = true, float radius = 14.0f,
                       ImU32 colour = IM_COL32(0, 0, 0, 40)) {
  if (!enable)
    return;

  ImDrawList* fg = ImGui::GetForegroundDrawList();
  ImGuiContext& ctx = *ImGui::GetCurrentContext();

  for (ImGuiWindow* w : ctx.Windows) {
    if (!w->WasActive || w->Hidden)
      continue;
    if (w->Flags & ImGuiWindowFlags_NoBackground)
      continue;

    // Skip root dock-space host by name
    if (strcmp(w->Name, "##MainDockHost") == 0)
      continue;

#ifdef IMGUI_HAS_DOCK
    if (w->DockNode || w->DockId)  // docked tabs & hosts
      continue;
#endif
    if (w->ParentWindow)  // child windows
      continue;

    const ImVec2 pad(radius, radius);
    ImVec2 min(w->Pos.x - pad.x, w->Pos.y - pad.y);
    ImVec2 max(w->Pos.x + w->Size.x + pad.x, w->Pos.y + w->Size.y + pad.y);

    fg->AddRectFilled(min, max, colour, radius);
  }
}
}  // namespace ui::shadow

#endif  // DROP_SHADOWS_H */
