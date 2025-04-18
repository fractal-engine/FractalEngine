#pragma once

#include <SDL.h>
#include <bgfx/bgfx.h>

namespace bgfx_macos {

void SetupPlatformData(bgfx::Init& init, SDL_Window* window);
}  // namespace bgfx_macos
