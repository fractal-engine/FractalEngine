#pragma once
#include <array>
#include <cstdint>

namespace ViewID {
// back-buffer
constexpr uint8_t UI_BACKGROUND = 0;

// scene FBO (colour+depth) – MUST be consecutive
constexpr uint8_t SCENE_SKYBOX = 1;
constexpr uint8_t SCENE_TERRAIN = 2;
constexpr uint8_t WATER_PASS = 3;
constexpr uint8_t DEBUG_PASS = 4;

// shadow map (separate FBO)
constexpr uint8_t SHADOW_PASS = 5;

// miscellaneous
constexpr uint8_t UI = 254;
constexpr uint8_t COUNT = 255;

/// every pass that is rendered **into scene_framebuffer_**
inline constexpr std::array<uint8_t, 4> kSceneViews = {
    SCENE_SKYBOX, SCENE_TERRAIN, WATER_PASS, DEBUG_PASS};
}  // namespace ViewID