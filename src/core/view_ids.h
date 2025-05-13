#pragma once
#include <cstddef>
#include <cstdint>

namespace ViewID {

constexpr uint8_t UI_BACKGROUND = 0; // default back-buffer - never point this at scene FBO

// main scene – *always* bound to GraphicsRenderer::scene_framebuffer_
constexpr uint8_t SCENE = 1;

// extra passes that still belong to scene FBO
inline constexpr uint8_t SCENE_N(std::size_t n) {
  return static_cast<uint8_t>(SCENE + n);
}

//define shadow pass using SCENE
constexpr uint8_t SHADOW_PASS = SCENE + 10;

constexpr uint8_t UI = 254; // IMGUI
constexpr uint8_t COUNT = 255;

}  // namespace ViewID
