#ifndef GAME_CANVAS_H
#define GAME_CANVAS_H

#include <imgui.h>

#include "editor/runtime/runtime.h"
#include "engine/core/engine_globals.h"
#include "engine/core/logger.h"
#include "engine/core/view_ids.h"
#include "engine/ecs/world.h"
#include "engine/renderer/model/model.h"
#include "engine/renderer/renderer_graphics.h"
#include "engine/resources/file_system_utils.h"
#include "editor/pipelines/scene_view_forward_pass.h"

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
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GLTF_FILE");
        if (payload) {
          // Get file path from payload
          std::string rel(static_cast<const char*>(payload->Data));

          // Convert to absolute path
          std::filesystem::path abs_path = Runtime::Project().AbsolutePath(rel);

          // Check extension before importing
          if (!FileSystem::HasExtension(abs_path, ".gltf") &&
              !FileSystem::HasExtension(abs_path, ".glb")) {
            Logger::getInstance().Log(
                LogLevel::Warning,
                "Dropped file is not a glTF model: " + abs_path.string());
          } else {
            Logger::getInstance().Log(
                LogLevel::Info, "Importing glTF model: " + abs_path.string());

            // Load meshes
            std::shared_ptr<Model> model = Model::Load(abs_path.string());

            // debug
            Logger::getInstance().Log(
                LogLevel::Debug,
                "[GameCanvas] Model loaded, mesh count: " +
                    std::to_string(model ? model->NLoadedMeshes() : 0));

            if (!model) {
              Logger::getInstance().Log(
                  LogLevel::Error,
                  "Import failed: no meshes in " + abs_path.string());
            } else {
              // Create ECS entity
              auto& world = ECS::Main();
              auto [entity, tr] =
                  world.CreateEntity(abs_path.filename().string());

              Logger::getInstance().Log(
                  LogLevel::Debug,
                  "[GameCanvas] Created entity: " + std::to_string((int)entity));

              // Attach a MeshRendererComponent for each mesh
              for (uint32_t i = 0; i < model->NLoadedMeshes(); ++i) {
                const Mesh* mesh = model->QueryMesh(i);
                if (!mesh)
                  continue;

                MeshRendererComponent& mr =
                    world.Add<MeshRendererComponent>(entity);
                mr.mesh_ = mesh;
                mr.enabled_ = true;
              }

              // Keep model alive for the lifetime of the scene
              static std::vector<std::shared_ptr<Model>> asset_cache;
              asset_cache.emplace_back(std::move(model));
            }
          }
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