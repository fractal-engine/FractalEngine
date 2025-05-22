#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace ViewID {
// ---------- back-buffer ----------
constexpr uint8_t UI_BACKGROUND = 0;  // clear colour etc.

// ---------- scene FBO (main colour+depth) ----------
constexpr uint8_t SCENE = 1;  // skybox / clear
constexpr uint8_t SCENE_EXTRA_BASE = SCENE + 1;

// helper: contiguous extra passes that still render into the scene FBO
inline constexpr uint8_t SceneExtra(std::size_t n) {
  return static_cast<uint8_t>(SCENE_EXTRA_BASE + n);
}

// shadow subsystem keeps its own block; large gap so we can grow later
constexpr uint8_t SHADOW_BASE = 32;
inline constexpr uint8_t Shadow(std::size_t n) {
  return static_cast<uint8_t>(SHADOW_BASE + n);
}

// ---------- UI ----------
constexpr uint8_t UI = 254;
constexpr uint8_t COUNT = 255;

// ---------- pass sets (update here, nowhere else) ----------
constexpr std::array<uint8_t, 3> kSceneViews = {
    SCENE,          // 0 – clear & sky
    SceneExtra(0),  // 1 – terrain & opaque
    SceneExtra(1)   // 2 – post-FX / debug / gizmos
};

constexpr std::array<uint8_t, 1> kShadowViews = {
    Shadow(0)  // first shadow-map
};
}  // namespace ViewID
