#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <SDL2/SDL_syswm.h>

#include "base/logger.h"
#include "platform/window_macos.h"

extern "C" {

void* WindowManager_CreateMetalLayer(void* cocoaWindow)
{
    NSWindow* nsWindow = (__bridge NSWindow*)cocoaWindow;
    NSView* contentView = [nsWindow contentView];

    // Make the view layer-backed
    [contentView setWantsLayer:YES];

    // Create Metal layer
    CAMetalLayer* metalLayer = [CAMetalLayer layer];
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.framebufferOnly = YES;
    metalLayer.contentsScale = [nsWindow backingScaleFactor];

    // Assign layer
    [contentView setLayer:metalLayer];

    Logger::getInstance().Log(
        LogLevel::Debug,
        "Metal layer successfully set up for window"
    );

    return (__bridge void*)metalLayer;
}

float WindowManager_GetDPIScale(SDL_Window* sdlWindow)
{
    int logicalW, logicalH;
    SDL_GetWindowSize(sdlWindow, &logicalW, &logicalH);

    int drawableW, drawableH;
    SDL_Metal_GetDrawableSize(sdlWindow, &drawableW, &drawableH);

    float scale = logicalW > 0 ? (float)drawableW / (float)logicalW : 1.0f;
    
    Logger::getInstance().Log(
        LogLevel::Debug,
        std::string("Calculated retina scale factor: ") + std::to_string(scale)
    );
    return scale;
}

void WindowManager_GetDrawableSize(SDL_Window* window, int* out_w, int* out_h) {
    SDL_Metal_GetDrawableSize(window, out_w, out_h);
}

} // extern "C"