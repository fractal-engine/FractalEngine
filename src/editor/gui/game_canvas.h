#ifndef GAME_CANVAS_H
#define GAME_CANVAS_H

#include <imgui.h>

#include "editor/runtime/application.h"
#include "engine/core/engine_globals.h"
#include "engine/core/logger.h"
#include "engine/core/view_ids.h"
#include "engine/formats/model_import.h"
#include "engine/renderer/renderer_graphics.h"

namespace Panels {

// calculate squared distance between two vectors
// Avoids rapid FBO recreation while resizing scene window
inline float DistanceSqr(const ImVec2& delta) {
  return delta.x * delta.x + delta.y * delta.y;
}

// Resize is deferred until the viewport has stabilized
inline void GameCanvas(bool isGameRunning, bool& hovered) {
  ImGui::BeginChild("GameCanvas", ImVec2(0, ImGui::GetContentRegionAvail().y),
                    true);

  ImVec2 pos = ImGui::GetCursorScreenPos();  // Get current cursor position
  ImVec2 size = ImGui::GetContentRegionAvail();
  ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;

  // Always update position immediately
  canvasViewportX = static_cast<uint16_t>(pos.x * scale.x);
  canvasViewportY = static_cast<uint16_t>(pos.y * scale.y);

  // Store raw size before applying
  ImVec2 proposedSize = size;
  ImVec2 scaledSize =
      ImVec2(proposedSize.x * scale.x, proposedSize.y * scale.y);

  // Update if dimensions have stabilized
  static ImVec2 lastSize = ImVec2(0, 0);
  static int stableFrames = 0;

  if (DistanceSqr(ImVec2(scaledSize.x - lastSize.x,
                         scaledSize.y - lastSize.y)) < 4.0f) {
    stableFrames++;
  } else {
    stableFrames = 0;
  }
  lastSize = scaledSize;

  if (stableFrames >= 2) {
    canvasViewportW = static_cast<uint16_t>(scaledSize.x);
    canvasViewportH = static_cast<uint16_t>(scaledSize.y);
  }

  hovered = ImGui::IsWindowHovered();  // hover detection for the canvas

  auto* renderer = static_cast<GraphicsRenderer*>(Application::Renderer());

  if (isGameRunning && canvasViewportW > 0 && canvasViewportH > 0) {
    // render when we have valid dimensions
    ImTextureID texId = renderer->GetSceneTexId();
    if (texId) {
      ImGui::Image(texId, size,
                   ImVec2(0, 1),  // uv0 (flips vertically for GL/Metal)
                   ImVec2(1, 0)   // uv1
      );

      // Handle drop operations for glTF files after drawing image
      if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GLTF_FILE");
        if (payload) {
          // Get file path from payload
          std::string path_str(static_cast<const char*>(payload->Data));
          std::filesystem::path rel_path(path_str);

          // Convert to absolute path
          std::filesystem::path abs_path =
              Application::Project().AbsolutePath(rel_path);

          Logger::getInstance().Log(
              LogLevel::Info, "Importing glTF model: " + abs_path.string());

          // Load and spawn model
          GltfImport::LoadModelAndSpawn(abs_path.string());
        }
        ImGui::EndDragDropTarget();
      }

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

}  // namespace Panels

#endif  // GAME_CANVAS_H