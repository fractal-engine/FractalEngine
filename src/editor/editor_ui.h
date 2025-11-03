#ifndef EDITOR_UI_H
#define EDITOR_UI_H

#include <entt/entt.hpp>
#include <memory>
#include "editor/camera/god_camera.h"
#include "editor/gizmos/component_gizmos.h"
#include "editor_base.h"
#include "engine/renderer/graphics_renderer.h"
#include "game/game_test.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "platform/input/key_map_sdl.h"

class EditorUI : public EditorBase {
public:
  explicit EditorUI(RendererBase* renderer);
  ~EditorUI();

  static EditorUI* Get();  // Singleton access

  void Initialize();
  void Run() override;
  void RequestUpdate() override;
  void Destroy() override;
  void BeginImGuiFrame(SDL_Window* window);

  // selection API
  void SetSelectedEntity(Entity entity);
  Entity GetSelectedEntity() const;
  Entity GetLastSelectedEntity() const;

private:
  void HandleInput(Key key);
  void RenderUI();
  void DockSpace();
  void LoadIcons();
  void UpdateMovement();

  RendererBase* renderer_ = nullptr;
  bool quit_ = false;
  bool is_game_started_ = true;
  bool game_canvas_hovered_ = false;
  bool game_canvas_focused_ = false;
  bool built_layout_ = false;  // guard for BuildDefaultLayout()

  // back-store for selection
  // entt::null is the type-safe way to say "nothing is selected"
  Entity selected_entity_ = entt::null;
  Entity last_selected_entity_ = entt::null;

  // ImGui docking and window flags
  ImGuiDockNodeFlags dock_flags_;
  ImGuiWindowFlags host_flags_;
  ImGuiID dock_id_;

  // singleton pointer
  static EditorUI* s_instance_;

  // ImGui debug controls
  bool debug_highlight_ids_ = false;
  bool debug_show_metrics_ = false;
  bool debug_show_log_ = false;
  bool debug_activate_picker_ = false;
  bool debug_show_style_editor_ = false;

  // Map component names to icons
  std::unordered_map<std::string, std::string> tab_icons_;

  GodCameraState god_state_{};
};

#endif  // EDITOR_UI_H