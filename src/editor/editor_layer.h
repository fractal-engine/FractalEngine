#ifndef EDITOR_LAYER_H
#define EDITOR_LAYER_H

#include <memory>
#include "editor/editor_base.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "renderer/renderer_graphics.h"
#include "subsystem/input/key_map_sdl.h"

class EditorLayer : public EditorBase {
public:
  explicit EditorLayer(std::unique_ptr<RendererBase>& renderer);
  ~EditorLayer();

  static EditorLayer* Get();  // Singleton access

  void Initialize();
  void Run() override;
  void RequestUpdate() override;
  void Shutdown() override;
  void BeginImGuiFrame(SDL_Window* window);

  // selection API
  void SetSelectedEntity(int id);
  int GetSelectedEntity() const;
  int GetLastSelectedEntity() const;

private:
  void HandleInput(Key key);
  void RenderUI();
  void DockSpace();
  void LoadIcons();

  std::unique_ptr<RendererBase>& renderer_;
  bool quit_ = false;
  bool is_game_started_ = false;
  bool game_canvas_hovered_ = false;
  bool built_layout_ = false;  // guard for BuildDefaultLayout()

  // back-store for selection
  int selected_entity_ = -1;
  int last_selected_entity_ = -1;

  ImGuiDockNodeFlags dock_flags_;
  ImGuiWindowFlags host_flags_;
  ImGuiID dock_id_;

  // singleton pointer
  static EditorLayer* s_instance_;

  // ImGui debug controls
  bool debug_highlight_ids_ = false;
  bool debug_show_metrics_ = false;
  bool debug_show_log_ = false;
  bool debug_activate_picker_ = false;

  // Map component names to icons
  std::unordered_map<std::string, std::string> tab_icons_;
};

#endif  // EDITOR_LAYER_H