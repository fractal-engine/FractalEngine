#ifndef EDITOR_BASE_H
#define EDITOR_BASE_H

#include <boost/signals2/signal.hpp>

#include "platform/input/input_event.h"

class EditorBase {

public:
  // virtual destructor
  virtual ~EditorBase() = default;

  void virtual Run() = 0;
  void virtual RequestUpdate() = 0;
  virtual void Destroy() = 0;
};

#endif  // EDITOR_BASE_H
