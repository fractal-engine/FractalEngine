#ifndef SEARCH_POPUP_H
#define SEARCH_POPUP_H

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <entt/entt.hpp>

#include "engine/ecs/world.h"

namespace SearchPopup {

// Render search popup
void Render();

// Close search popup
void Close();

// Open search popup at position for ecs components
void SearchComponents(ImVec2 position, Entity target);

}  // namespace SearchPopup

#endif  // SEARCH_POPUP_H