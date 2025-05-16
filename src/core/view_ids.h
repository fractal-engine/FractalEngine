#pragma once
#include <cstddef>
#include <cstdint>

namespace ViewID {
// default back-buffer - never point this at scene FBO
constexpr uint8_t UI_BACKGROUND = 0;

// main scene – *always* bound to GraphicsRenderer::scene_framebuffer_
constexpr uint8_t SCENE = 1;

// extra passes that still belong to scene FBO
inline constexpr uint8_t SCENE_N(std::size_t n) {
  return static_cast<uint8_t>(SCENE + n);
}

constexpr uint8_t UI = 254;  // ImGui
constexpr uint8_t COUNT = 255;
}  // namespace ViewID
