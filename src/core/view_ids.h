#ifndef VIEW_IDS_H
#define VIEW_IDS_H

#include <cstddef>
#include <cstdint>

namespace ViewID {
enum : uint8_t { SCENE = 1, UI_BACKGROUND = 0, UI = 2, COUNT };

inline constexpr uint8_t GAME_N(std::size_t n) {
  return static_cast<uint8_t>(SCENE + n);
}
}  // namespace ViewID

#endif  // VIEW_IDS_H
