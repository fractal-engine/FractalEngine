#ifndef GAME_CANVAS_H
#define GAME_CANVAS_H

#include <imgui.h>

#include <filesystem>
#include "editor/pipelines/scene_view_forward_pass.h"
#include "editor/editor_ui.h"
#include "editor/runtime/runtime.h"
#include "engine/core/engine_globals.h"
#include "engine/core/logger.h"
#include "engine/core/view_ids.h"
#include "engine/ecs/world.h"
#include "engine/renderer/model/model.h"
#include "engine/renderer/renderer_graphics.h"
#include "engine/resources/file_system_utils.h"

// TODO: rename window to scene_view_window

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

  // --- NEW: UNIFIED CAMERA INPUT HANDLING ---
  if (hovered) {
    // Get the one true editor camera.
    OrbitCamera& camera = EditorUI::Get()->GetCamera();
    ImGuiIO& io = ImGui::GetIO();

    // 1. Mouse Orbit (Middle Mouse Button)
    if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
      camera.orbit(io.MouseDelta.x, io.MouseDelta.y);
    }

    // 2. Mouse Zoom (Mouse Wheel)
    if (io.MouseWheel != 0.0f) {
      camera.zoom(io.MouseWheel);
    }

    // 3. Mouse Pan (Right Mouse Button)
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
      camera.pan(io.MouseDelta.x, io.MouseDelta.y);
    }

    // 4. Keyboard Pan (from the old CameraSystem)
    // You can use ImGui::IsKeyDown() for a smoother feel.
    if (ImGui::IsKeyDown(ImGuiKey_W))
      camera.pan(0.0f, 5.0f);  // Adjusted speed
    if (ImGui::IsKeyDown(ImGuiKey_S))
      camera.pan(0.0f, -5.0f);
    if (ImGui::IsKeyDown(ImGuiKey_A))
      camera.pan(-5.0f, 0.0f);
    if (ImGui::IsKeyDown(ImGuiKey_D))
      camera.pan(5.0f, 0.0f);
  }

  auto* renderer = static_cast<GraphicsRenderer*>(Runtime::Renderer());

  if (isGameRunning && canvasViewportW > 0 && canvasViewportH > 0) {
    // render when we have valid dimensions
    ImTextureID texId = renderer->GetSceneTexId();
    if (texId) {
      ImGui::Image(texId, size, ImVec2(0, 1),  // uv0
                   ImVec2(1, 0)                // uv1
      );

      // Handle drop operations for glTF files after drawing image
      if (ImGui::BeginDragDropTarget()) {
        // The payload type "GLTF_FILE" must match the one in asset_browser.cpp
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GLTF_FILE");
        if (payload) {
          std::filesystem::path sourcePath = (const char*)payload->Data;

          // 1. Get the destination path within project's assets folder.
          //    implement GetAssetsPath() in ProjectManager.
          std::filesystem::path destPath =
              Runtime::Project().GetAssetsPath() / sourcePath.filename();

          try {
            // 2. The handler's ONLY job is to copy the file. This is a safe
            // operation.
            std::filesystem::copy_file(
                sourcePath, destPath,
                std::filesystem::copy_options::overwrite_existing);
            Logger::getInstance().Log(
                LogLevel::Info,
                "Asset dropped and copied to: " + destPath.string());

          } catch (const std::filesystem::filesystem_error& e) {
            Logger::getInstance().Log(
                LogLevel::Error,
                "Failed to copy dropped asset: " + std::string(e.what()));
          }

          // 3. We do NOT load the model or create any entities here.
          //    The ProjectObserver will now see the new file and queue an
          //    event. The main loop's PollEvents() call will safely process
          //    that event.
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