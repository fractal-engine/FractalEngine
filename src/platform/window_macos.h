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
 * Typically by creating an SDL_MetalView, getting the CAMetalLayer, etc.
 *
 * @param sdlWindow The SDL_Window*
 * @return The scale factor, e.g. 2.0 on a high-DPI (Retina) Mac
 */
float WindowManager_GetDPIScale(SDL_Window* sdlWindow);

}  // extern "C"
