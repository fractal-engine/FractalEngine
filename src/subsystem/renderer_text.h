#ifndef RENDERER_TEXT_H
#define RENDERER_TEXT_H

#include <boost/signals2/signal.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/canvas.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <mutex>
#include <string>

#include "base/renderer_base.h"

class RendererText : public RendererBase {
  friend class SubsystemManager;  // Allow SubsystemManager to access private
                                  // members

private:
  ftxui::Canvas canvas_;
  ftxui::Component game_canvas_;
  RendererText();  // Private constructor
  void Render() override {}
  std::mutex canvas_mutex_;

  void showText(const std::string& text, int x, int y);

public:
  void SetSize(int width, int height) override;
  void ClearDisplay() override;
  void ShowText(const std::string& text, int x, int y) override;
  void ShowText(const std::vector<std::string>& text_area, int x,
                int y) override;
  const ftxui::Component& GetGameCanvas() const;
};

#endif  // RENDERER_TEXT_H
