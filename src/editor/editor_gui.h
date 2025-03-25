#ifndef EDITOR_GUI_H
#define EDITOR_GUI_H

#include <memory>
#include <mutex>

#include "base/editor_base.h"
#include "imgui.h"

class RendererBase;

class EditorGUI : public EditorBase {
  friend class SubsystemManager;

private:
  std::unique_ptr<RendererBase>& renderer_;
  bool quit_;
  bool is_game_started_;  // Track if game is started

  // Game canvas position and size
  ImVec2 gameCanvasPos_;
  ImVec2 gameCanvasSize_;

  // Game canvas hover state
  bool gameCanvasHovered_ = false;

  // virtual destructor
  virtual ~EditorGUI();

  // Constructor
  EditorGUI(std::unique_ptr<RendererBase>& renderer);

public:
  // Runs main loop on main thread (SDL event loop)
  void Run() override;
  void RequestUpdate() override;
  void game_inputed(Key key);  // Handles keyboard input
};

#endif  // EDITOR_GUI_H
