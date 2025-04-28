#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <imgui.h>
#include <functional>

namespace Components {

struct ToolbarCallbacks {
  std::function<void()> onStart;
  std::function<void()> onStop;
  std::function<void()> onAddObject;
  std::function<void()> onRemoveObject;
  std::function<void()> onQuit;
};

inline void Toolbar(const ToolbarCallbacks& cb) {
  if (ImGui::Button("Start") && cb.onStart)
    cb.onStart();
  ImGui::SameLine();
  if (ImGui::Button("Stop") && cb.onStop)
    cb.onStop();
  ImGui::SameLine();
  if (ImGui::Button("Add GameObject") && cb.onAddObject)
    cb.onAddObject();
  ImGui::SameLine();
  if (ImGui::Button("Remove GameObject") && cb.onRemoveObject)
    cb.onRemoveObject();
  ImGui::SameLine();
  if (ImGui::Button("Quit") && cb.onQuit)
    cb.onQuit();
  ImGui::Separator();
}

}  // namespace Components

#endif  // TOOLBAR_H