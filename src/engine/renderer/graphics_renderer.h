#ifndef GRAPHICS_RENDERER_H
#define GRAPHICS_RENDERER_H

#include "renderer_base.h"
#include <SDL.h>
#include <vector>
#include <cstdint>
#include <bgfx/bgfx.h>

class GraphicsRenderer : public RendererBase {
public:
    GraphicsRenderer();
    virtual ~GraphicsRenderer();

    // --- Lifecycle ---
    bool Initialize(SDL_Window* window);
    void BeginFrame();
    void EndFrame() override; // This will now work after you fix renderer_base.h
    void Shutdown() override;

    // --- Generic Resource Management ---
    bgfx::FrameBufferHandle CreateFrameBuffer(const std::vector<bgfx::TextureHandle>& attachments, bool destroyTextures = false);
    void DestroyFrameBuffer(bgfx::FrameBufferHandle& fbh);
    
    // This declaration will now compile correctly
    bgfx::TextureHandle CreateTexture2D(uint16_t width, uint16_t height, bgfx::TextureFormat::Enum format, uint64_t flags);
    void DestroyTexture(bgfx::TextureHandle& th);

    // --- View Management ---
    uint16_t ReserveViewId();

private:
    void OnResize(uint16_t width, uint16_t height);

    SDL_Window* window_ = nullptr;
    uint16_t view_id_counter_ = 0;
};

#endif // GRAPHICS_RENDERER_H