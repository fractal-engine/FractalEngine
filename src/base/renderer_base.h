#ifndef RENDERER_BASE_H
#define RENDERER_BASE_H

#include "base/singleton.hpp"

#include <boost/signals2/signal.hpp>
#include <memory>
#include <string>
#include <vector>
#include "base/logger.h"

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

  int GetHeight() const;
  int GetWidth() const;

  virtual void SetSize(int w, int h);
  virtual void ClearDisplay() = 0;
  virtual void ShowText(const std::string& text, int x, int y) = 0;
  virtual void ShowText(const std::vector<std::string>& text_area, int x,
                        int y) = 0;

  boost::signals2::signal<void()> redrawn;
};

#endif  // RENDERER_BASE_H

