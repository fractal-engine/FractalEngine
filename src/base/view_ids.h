#ifndef VIEW_IDS_H
#define VIEW_IDS_H

namespace ViewID {

// reserved view IDs
constexpr uint8_t CLEAR = 0;            // Clears screen (background color)
constexpr uint8_t GAME = 1;             // Game rendering view
constexpr uint8_t GAME_DEBUG = 2;       // Optional debug overlays
constexpr uint8_t UI_BACKGROUND = 254;  // Background for UI
constexpr uint8_t UI = 255;             // ImGui and all GUI rendering

// Dynamic views
inline constexpr uint8_t GAME_N(size_t n) {
  return static_cast<uint8_t>(GAME + n);
}

}  // namespace ViewID

#endif  // VIEW_IDS_H