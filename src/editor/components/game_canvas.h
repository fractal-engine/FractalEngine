#ifndef GAME_CANVAS_H
#define GAME_CANVAS_H

#include <imgui.h>
#include "core/engine_globals.h"
#include "core/view_ids.h"
#include "renderer/renderer_graphics.h"
#include "subsystem/subsystem_manager.h"

namespace Components {

inline void GameCanvas(bool isGameRunning, bool& hovered) {
  ImGui::BeginChild("GameCanvas", ImVec2(0, ImGui::GetContentRegionAvail().y),
                    true);

  ImVec2 pos = ImGui::GetCursorScreenPos();  // Get current cursor position
  ImVec2 size = ImGui::GetContentRegionAvail();
  ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;

  // Set global viewport dimensions used by the game/renderer
  canvasViewportX = static_cast<uint16_t>(pos.x * scale.x);
  canvasViewportY = static_cast<uint16_t>(pos.y * scale.y);
  canvasViewportW = static_cast<uint16_t>(size.x * scale.x);
  canvasViewportH = static_cast<uint16_t>(size.y * scale.y);

  hovered = ImGui::IsWindowHovered();  // hover detection for the canvas

  auto* renderer =
      static_cast<GraphicsRenderer*>(SubsystemManager::GetRenderer().get());

  if (isGameRunning && canvasViewportW > 0 && canvasViewportH > 0) {
    // try to render when we have valid dimensions
    ImTextureID texId = renderer->GetSceneTexId();
    if (texId) {
      ImGui::Image(texId, size,
                   ImVec2(0, 1),  // uv0 (flips vertically for GL/Metal)
                   ImVec2(1, 0)   // uv1
      );
    } else {
      ImGui::TextColored(ImVec4(1, 0, 0, 1), "Texture ID invalid!");
    }
  } else {
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), isGameRunning
                                                        ? "Resizing viewport..."
                                                        : "Game not started");
  }

  ImGui::EndChild();
}

}  // namespace Components

#endif  // GAME_CANVAS_H