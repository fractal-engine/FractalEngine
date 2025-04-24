#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <SDL2/SDL_syswm.h>
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"

#include "platform_utils.h"
#include "core/logger.h"

#if defined(__APPLE__)

namespace platform {

void GetDrawableSize(SDL_Window* window, int* out_w, int* out_h) {
    SDL_Metal_GetDrawableSize(window, out_w, out_h);
}

float GetDPIScale(SDL_Window* window) {
    int logicalW, logicalH;
    SDL_GetWindowSize(window, &logicalW, &logicalH);

    int drawableW, drawableH;
    SDL_Metal_GetDrawableSize(window, &drawableW, &drawableH);

    float scale = logicalW > 0 ? (float)drawableW / (float)logicalW : 1.0f;
    
    // Logger::getInstance().Log(LogLevel::Debug, "macOS DPI Scale: " + std::to_string(scale)); // DEBUG only
    return scale;
}

void* CreateMetalLayer(void* cocoaWindow) {
    NSWindow* nsWindow = (__bridge NSWindow*)cocoaWindow;
    NSView* contentView = [nsWindow contentView];
    [contentView setWantsLayer:YES];

    CAMetalLayer* metalLayer = [CAMetalLayer layer];
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.framebufferOnly = YES;
    metalLayer.contentsScale = [nsWindow backingScaleFactor];

    [contentView setLayer:metalLayer];

    Logger::getInstance().Log(LogLevel::Debug, "Created CAMetalLayer");
    return (__bridge void*)metalLayer;
}

void SetupBGFXPlatformData(bgfx::Init& init, SDL_Window* window) {
  int dw, dh;
  GetDrawableSize(window, &dw, &dh);
  init.resolution.width = dw;
  init.resolution.height = dh;
  init.resolution.reset = BGFX_RESET_VSYNC;

  SDL_SysWMinfo wmi;
  SDL_VERSION(&wmi.version);
  if (SDL_GetWindowWMInfo(window, &wmi)) {
    void* layer = CreateMetalLayer((__bridge void*)wmi.info.cocoa.window);
    init.platformData.nwh = layer;
    init.platformData.ndt = nullptr;
    init.platformData.context = nullptr;
  }
}

void InitSDLForImGui(SDL_Window* window) {
  ImGui_ImplSDL2_InitForMetal(window);
}

}  // namespace platform

#endif
