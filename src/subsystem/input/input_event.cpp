#include "subsystem/input/input_event.h"

InputEvent::InputEvent() : key_(Key::NONE) {}
InputEvent::InputEvent(Key key) : key_(key) {}