#ifndef EDITOR_BASE_H
#define EDITOR_BASE_H

#include <boost/signals2/signal.hpp>
#include "platform/input/input_event.h"

// Forward declare SDL_Event so we don't need to include the full SDL header
union SDL_Event;

class EditorBase {
public:
  virtual ~EditorBase() = default;

  // --- Edited New Interface ---
  // It matches the new responsibilities of the EditorUI class.
  virtual void Initialize() = 0;
  virtual void Destroy() = 0;  // This was already correct

  virtual void HandleEvent(const SDL_Event& event) = 0;
  virtual void BeginFrame() = 0;
  virtual void RenderPanels() = 0;
  virtual void RenderDraws() = 0;

  // --- REMOVED OBSOLETE FUNCTIONS ---
  // void virtual Run() = 0; // The main loop is now in runtime.cpp
  // void virtual RequestUpdate() = 0; // Obsolete in a continuous main loop

  // Signals
  boost::signals2::signal<void()> game_start_pressed;
  boost::signals2::signal<void()> game_end_pressed;
  boost::signals2::signal<void()> editor_exit_pressed;
  boost::signals2::signal<void(InputEvent)> game_inputed;
};

#endif  // EDITOR_BASE_H