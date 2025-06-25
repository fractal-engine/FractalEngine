#pragma once
#include <array>
#include <cstdint>

namespace ViewID {
// view IDs must be unique per frame
constexpr uint8_t REFLECTION_PASS = 0;  // Render to reflection texture
constexpr uint8_t UI_BACKGROUND = 1;    // Back-buffer UI

// scene FBO (colour+depth) – MUST be consecutive
constexpr uint8_t SCENE_SKYBOX = 2;
constexpr uint8_t SCENE_TERRAIN = 3;
constexpr uint8_t WATER_PASS = 4;
constexpr uint8_t DEBUG_PASS = 5;

// shadow map (separate FBO)
constexpr uint8_t SHADOW_PASS = 6;

// For individual glTF game objects
constexpr uint8_t SCENE_MESH = 7;  

// miscellaneous
constexpr uint8_t UI = 254;
constexpr uint8_t COUNT = 255;

/// every pass that is rendered **into scene_framebuffer_**
inline constexpr std::array<uint8_t, 5> kSceneViews = {
    SCENE_SKYBOX, SCENE_TERRAIN, WATER_PASS, DEBUG_PASS, SCENE_MESH};

}  // namespace ViewID