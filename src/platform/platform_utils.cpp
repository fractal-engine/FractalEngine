#include "platform_utils.h"

#include <SDL.h>

#if defined(_WIN32) || defined(__linux__)

namespace platform {

void GetDrawableSize(SDL_Window* window, int* out_w, int* out_h) {
  SDL_GetRendererOutputSize(SDL_GetRenderer(window), out_w, out_h);
}

float GetDPIScale(SDL_Window* window) {
  int logicalW, logicalH;
  SDL_GetWindowSize(window, &logicalW, &logicalH);
  int drawableW, drawableH;
  SDL_GL_GetDrawableSize(window, &drawableW, &drawableH);
  return logicalW > 0 ? static_cast<float>(drawableW) / logicalW : 1.0f;
}

void* CreateMetalLayer(void* native_window) {
  // Not applicable on non-macOS
  return nullptr;
}

void SetupBGFXPlatformData(bgfx::Init& init, SDL_Window* window) {
  int w, h;
  SDL_GetWindowSize(window, &w, &h);
  init.resolution.width = w;
  init.resolution.height = h;
  init.resolution.reset = BGFX_RESET_VSYNC;

  SDL_SysWMinfo wmi;
  SDL_VERSION(&wmi.version);
  if (SDL_GetWindowWMInfo(window, &wmi)) {
#if defined(_WIN32)
    init.platformData.ndt = nullptr;
    init.platformData.nwh = wmi.info.win.window;
    init.platformData.context = nullptr;
#elif defined(__linux__)
    init.platformData.ndt = wmi.info.x11.display;
    init.platformData.nwh = (void*)(uintptr_t)wmi.info.x11.window;
    init.platformData.context = nullptr;
#endif
  }
}

void InitSDLForImGui(SDL_Window* window) {
  ImGui_ImplSDL2_InitForOther(window);
}

}  // namespace platform

#endif
