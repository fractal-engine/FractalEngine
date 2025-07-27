#ifndef EDITOR_BASE_H
#define EDITOR_BASE_H

#include <boost/signals2/signal.hpp>

#include "platform/input/input_event.h"

class EditorBase {

public:
  // virtual destructor
  virtual ~EditorBase() = default;

  // FIXME - We have to fix the visibility problem. Maybe using pass key pattern
  boost::signals2::signal<void()> game_start_pressed;
  boost::signals2::signal<void()> game_end_pressed;
  boost::signals2::signal<void()> editor_exit_pressed;
  boost::signals2::signal<void(InputEvent)> game_inputed;

  void virtual Run() = 0;
  void virtual RequestUpdate() = 0;
  virtual void Destroy() = 0;
};

#endif  // EDITOR_BASE_H
