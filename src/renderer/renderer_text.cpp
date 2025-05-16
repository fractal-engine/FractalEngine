#include "renderer/renderer_text.h"

#include <algorithm>

#include "core/logger.h"

using namespace ftxui;

// Constructor to initialize the canvas and game canvas component
RendererText::RendererText() {
  SetSize(100, 50);
  game_canvas_ = Renderer([&] {
    std::lock_guard<std::mutex> lock(canvas_mutex_);
    return canvas(canvas_);
  });
}

// Function to set the size of the canvas
void RendererText::SetSize(int width, int height) {
  RendererBase::SetSize(width, height);
  canvas_ = Canvas(width_, height_);
  Logger::getInstance().Log(LogLevel::Info, "RendererText set size at (" +
                                                std::to_string(width_) + ", " +
                                                std::to_string(height_) + ")");
}

// Function to clear the display by resetting the canvas
void RendererText::ClearDisplay() {
  std::lock_guard<std::mutex> lock(canvas_mutex_);
  canvas_ = Canvas(width_, height_);
  // redrawn();
}

// Function to show text on the canvas at a specific position
void RendererText::showText(const std::string& text, int x, int y) {
  std::lock_guard<std::mutex> lock(canvas_mutex_);
  canvas_.DrawText(x, y, text);
  redrawn();
}

// Function to show text on the canvas and trigger redraw
void RendererText::ShowText(const std::string& text, int x, int y) {
  showText(text, x, y);
}

// Function to show multiple lines of text on the canvas
void RendererText::ShowText(const std::vector<std::string>& text_area, int x,
                            int y) {
  int offset_y = 0;
  for (const std::string& str : text_area) {
    ShowText(str, x, y + offset_y);
    offset_y += 4;
  }
  redrawn();
}

const ftxui::Component& RendererText::GetGameCanvas() const {
  return game_canvas_;
}
