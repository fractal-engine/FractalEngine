#include "subsystem/window_manager.h"
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include "base/logger.h"

extern "C" {
    void* setupMetalLayer(void* window) {
        // Use __bridge to convert C pointer to Objective-C
        NSWindow* nsWindow = (__bridge NSWindow*)window;
        NSView* contentView = [nsWindow contentView];
        
        // Make the view layer-backed
        [contentView setWantsLayer:YES];
        
        // Create a Metal layer
        CAMetalLayer* metalLayer = [CAMetalLayer layer];
        
        // Set properties
        metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        metalLayer.framebufferOnly = YES;
        
        // High DPI support
        metalLayer.contentsScale = [nsWindow backingScaleFactor];
        
        // Set Metal layer as the view's backing layer
        [contentView setLayer:metalLayer];
        
        Logger::getInstance().Log(LogLevel::Debug, "Metal layer successfully set up for window");
        
        // Use __bridge to convert back to C pointer
        return (__bridge void*)nsWindow;
    }
}