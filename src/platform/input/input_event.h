#ifndef INPUT_EVENT_H
#define INPUT_EVENT_H

#include <cstdint>

#include "key.h"

struct InputEvent {
  Key key_;
  uint64_t pressed_frame_;
  bool is_pressed = true;
  InputEvent();
  InputEvent(Key key);
};

#endif