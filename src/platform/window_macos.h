#pragma once

#include <SDL.h>

extern "C" {

/**
 * Create a CAMetalLayer for the given Cocoa window pointer.
 *
 * @param cocoaWindow Pointer to the NSWindow from SDL_SysWMinfo
 * @return A void* pointer to the newly created CAMetalLayer
 */
void* WindowManager_CreateMetalLayer(void* cocoaWindow);

/**
 * Fetch the Retina scale factor from an SDL window.
 * @param sdlWindow The SDL_Window*
 * @return The scale factor, e.g. 2.0 on a high-DPI (Retina) Mac
 */
float WindowManager_GetDPIScale(SDL_Window* sdlWindow);

/**
 * SDL_Metal_GetDrawableSize returns the actual pixel
 * dimensions of the CAMetalLayer backing surface.
 */
void WindowManager_GetDrawableSize(SDL_Window* window, int* out_w, int* out_h);

}  // extern "C"
