#ifndef EDITOR_UI_H
#define EDITOR_UI_H

#include <entt/entt.hpp>  // Provides entt::entity
#include <memory>
#include "editor/gizmos/scene_view_gizmo.h"
#include "editor/gui/orbit_camera.h"
#include "editor_base.h"
#include "engine/renderer/renderer_graphics.h"
#include "game/game_test.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "platform/input/key_map_sdl.h"

// Forward declare SDL_Event
union SDL_Event;

// Use the Entity type alias for consistency
using Entity = entt::entity;

class EditorUI : public EditorBase {
public:
  explicit EditorUI(RendererBase* renderer);
  ~EditorUI();

  static EditorUI* Get();

  void Initialize();
  void Destroy();

  // --- THESE ARE THE NEW PUBLIC FUNCTIONS ---
  // They replace the old Run() method. The main loop in runtime.cpp will call
  // these.

  // Handles an incoming platform event (like mouse, keyboard, window).
  void HandleEvent(const SDL_Event& event);
  // Prepares ImGui for a new frame.
  void BeginFrame();
  // Renders all the editor's ImGui windows and panels.
  void RenderPanels();

  // selection API
  void SetSelectedEntity(Entity entity);
  Entity GetSelectedEntity() const;
  Entity GetLastSelectedEntity() const;
  OrbitCamera& GetCamera() { return camera_; }

private:
  void HandleInput(Key key);
  void RenderUI();
  void DockSpace();
  void LoadIcons();

  RendererBase* renderer_ = nullptr;
  bool quit_ = false;
  bool is_game_started_ = false;
  bool game_canvas_hovered_ = false;
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

  // Orbit camera for scene view
  OrbitCamera camera_;
};

#endif  // EDITOR_UI_H