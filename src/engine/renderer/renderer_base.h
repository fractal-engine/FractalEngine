#ifndef RENDERER_BASE_H
#define RENDERER_BASE_H

#include "engine/core/singleton.hpp"
#include "engine/core/logger.h"

#include <boost/signals2/signal.hpp>
#include <memory>
#include <string>
#include <vector>

class RendererBase;

class RendererBase {
protected:
  int height_, width_;
  RendererBase() = default;

public:
  // virtual destructor
  virtual ~RendererBase() = default;

  virtual void Render() = 0;
  virtual void Shutdown() = 0;

  boost::signals2::signal<void()> redrawn;
};

#endif  // RENDERER_BASE_H
