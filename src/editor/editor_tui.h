#ifndef EDITOR_TUI_H
#define EDITOR_TUI_H

#include <boost/signals2/signal.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>
#include <thread>
#include <vector>

#include "base/editor_base.h"
#include "subsystem/renderer_text.h"

class EditorTUI : public EditorBase {
  friend class SubsystemManager;

private:
  ftxui::ScreenInteractive screen_;
  ftxui::Component editor_interactive_;  // Input handler for the editor
  ftxui::Component editor_layout_;
  std::vector<std::string> log_entries_;  // Log entries to be displayed

  // Renderer which will return the game canvas back to the editor.
  const ftxui::Component& game_canvas_;
  const ftxui::Component game_interactive_;  // forward input here
  ftxui::Component renderLayout();           // Renamed function
  ftxui::Component addInteraction();

  // Settings for game canvas
  int game_canvas_height_, game_canvas_width_;
  bool is_game_start_ = false;

  // Member variables for control buttons and debug renderer
  ftxui::Component control_buttons_;
  ftxui::Component debug_renderer_;
  // Member variables for scroll position
  float scroll_x = 0.0;
  float scroll_y = 0.0;
  ftxui::Component scrollbar_y;

  EditorTUI() = delete;
  EditorTUI(const std::unique_ptr<RendererBase>& renderer);

public:
  void Run() override;
  void RequestUpdate() override;
  void ReadLogFile();
};

#endif  // EDITOR_TUI_H
