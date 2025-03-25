#include "base/renderer_base.h"

#include "base/logger.h"

// void DisplayEngine::initialize(std::unique_ptr<DisplayBase>&& displayer) {
//   if (displayer == nullptr) {
//     std::cerr << "Displayer Not Provided!\n";
//     Logger::getInstance().log(
//         LogLevel::ERROR,
//         "Displayer Not Provided!");  // specify level then message
//     return;
//   }
//   if (getInstance().displayer_) {
//     std::cerr << "Display Engine Already Initialized!\n";
//     Logger::getInstance().log(LogLevel::WARNING,
//                               "Display Engine Already Initialized!");
//     return;
//   }
//   getInstance().displayer_ = std::move(displayer);
//   Logger::getInstance().log(LogLevel::INFO,
//                             "Display Engine Initialization Successful.");
// }
// const std::unique_ptr<DisplayBase>& DisplayEngine::getDisplayer() {
//   return getInstance().displayer_;
// }

int RendererBase::GetHeight() const {
  return height_;
}

int RendererBase::GetWidth() const {
  return width_;
}

void RendererBase::SetSize(int w, int h) {
  width_ = w;
  height_ = h;
  Logger::getInstance().Log(LogLevel::INFO, "DisplayEngine set size at (" +
                                                std::to_string(width_) + ", " +
                                                std::to_string(height_) + ")");
}
