#ifndef INPUT_H
#define INPUT_H
#include <boost/signals2/signal.hpp>
#include <mutex>
#include <vector>
#include "platform/input/input_event.h"

class Input {
private:
  std::vector<InputEvent> buffered_events_;
  std::vector<Key> just_pressed_keys_;
  std::vector<Key> just_released_keys_;
  std::vector<Key> pressed_keys_;
  std::mutex event_mutex_;

  uint64_t current_frame_ = 0;
  std::mutex buffer_mutex_;

public:
  Input() = default;
  bool IsJustPressed(const Key& key) const;
  bool IsPressed(const Key& key) const;
  void FowardInputEvent(InputEvent event, uint64_t frame);
  void FlushInputEvent();
};

#endif  // INPUT_H