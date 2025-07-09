#include "input.h"
#include "editor/runtime/application.h"

bool Input::IsPressed(const Key& key) const {
  // Not supported
  return false;
}

bool Input::IsJustPressed(const Key& key) const {
  for (const InputEvent& event : buffered_events_) {
    if (event.key_ == key &&
        event.pressed_frame_ + 1 == Application::Game()->GetFrameCount()) {
      return true;
    }
  }
  return false;
}

void Input::FowardInputEvent(InputEvent event, uint64_t frame) {
  std::lock_guard<std::mutex> lock(event_mutex_);
  for (InputEvent& buffered_event : buffered_events_) {
    if (buffered_event.key_ == event.key_) {
      buffered_event.pressed_frame_ = frame;
      return;
    }
  }

  buffered_events_.push_back(event);
}

void Input::FlushInputEvent() {}