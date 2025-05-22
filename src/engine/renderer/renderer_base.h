#ifndef RENDERER_BASE_H
#define RENDERER_BASE_H

#include "engine/core/singleton.hpp"
#include "engine/core/logger.h"

#include <boost/signals2/signal.hpp>
#include <memory>
#include <string>
#include <vector>

class RendererBase;

// class DisplayEngine : public Singleton<DisplayEngine> {
// private:
//   std::unique_ptr<DisplayBase> displayer_;

// public:
//   ~DisplayEngine() = default;
//   static void initialize(std::unique_ptr<DisplayBase>&& displayer);
//   static const std::unique_ptr<DisplayBase>& getDisplayer();
// };

class RendererBase {
protected:
  int height_, width_;
  RendererBase() = default;

public:
  // virtual destructor
  virtual ~RendererBase() = default;

  virtual void Render() = 0;
  virtual void ClearDisplay() = 0;
  virtual void ShowText(const std::string& text, int x, int y) = 0;
  virtual void ShowText(const std::vector<std::string>& text_area, int x,
                        int y) = 0;
  virtual void Shutdown() = 0;

  virtual void SetSize(int w, int h);
  int GetHeight() const;
  int GetWidth() const;

  boost::signals2::signal<void()> redrawn;
};

#endif  // RENDERER_BASE_H
