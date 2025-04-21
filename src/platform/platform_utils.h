#pragma once

#include <SDL.h>
#include <bgfx/bgfx.h>

namespace platform {

/**
 * Returns the pixel size of the drawable area (HiDPI aware).
 */
void GetDrawableSize(SDL_Window* window, int* out_w, int* out_h);

/**
 * Returns the DPI scaling factor of the window.
 * Default is 1.0f on non-HiDPI or unsupported platforms.
 */
float GetDPIScale(SDL_Window* window);

/**
 * macOS: Creates and attaches a CAMetalLayer to a Cocoa window.
 * Other platforms: returns nullptr.
 */
void* CreateMetalLayer(void* native_window);

void SetupBGFXPlatformData(bgfx::Init& init, SDL_Window* window);

void InitSDLForImGui(SDL_Window* window);

}  // namespace platform
