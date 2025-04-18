#import "platform/window_macos.h"
#import "bgfx_macos.h"
#import <SDL2/SDL_syswm.h>
#include "base/logger.h"

namespace bgfx_macos {

void SetupPlatformData(bgfx::Init& init, SDL_Window* window) {
    // 1) Query drawable-size and apply to init.resolution
    int dw, dh;
    WindowManager_GetDrawableSize(window, &dw, &dh);
    init.resolution.width  = dw;
    init.resolution.height = dh;
    init.resolution.reset  = BGFX_RESET_VSYNC;

    // 2) Get the CAMetalLayer via the Cocoa bridge
    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (SDL_GetWindowWMInfo(window, &wmi)) {
      void* layer = WindowManager_CreateMetalLayer((__bridge void *)wmi.info.cocoa.window);
      init.platformData.nwh     = layer;
      init.platformData.ndt     = nullptr;
      init.platformData.context = nullptr;
      Logger::getInstance().Log(LogLevel::Debug,
        "BGFX macOS metal layer = " + std::to_string((uintptr_t)layer));
    } else {
      Logger::getInstance().Log(LogLevel::Error,
        "SDL_GetWindowWMInfo failed for macOS");
    }
}

} // namespace bgfx_macos
