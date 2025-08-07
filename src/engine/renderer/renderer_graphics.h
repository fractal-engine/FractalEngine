#ifndef RENDERER_GRAPHICS_H
#define RENDERER_GRAPHICS_H

#include <SDL.h>
#include <imgui.h>

#include <mutex>
#include <string>
#include <vector>

#include "renderer_base.h"

#include "platform/platform_utils.h"
#include "platform/window_manager.h"

class GraphicsRenderer : public RendererBase {
public:
  // The constructor takes a mutex reference
  GraphicsRenderer();
  virtual ~GraphicsRenderer();  // Virtual destructor

  // Initializes BGFX
  bool InitBgfx();

  // RendererBase interface
  void ClearDisplay() override;
  void ShowText(const std::string& text, int x, int y) override;
  void ShowText(const std::vector<std::string>& text_area, int x,
                int y) override;

  void RenderGameContent();
  void ConfigureViews();
  void PrepareFrame();
  void BeginImGuiFrame();
  void EndImGuiFrame();
  void CreateFramebuffers(uint16_t w, uint16_t h);
  void SetSize(int w, int h) override;
  void UpdateCanvasSize(uint16_t w, uint16_t h);

  // Called after drawing to present the frame.
  void Render() override;

  // Processes SDL events, including quitting – called from the Editor loop.
  void ProcessEvents(bool& quit);

  void InitShaders();

  void Shutdown() override;

  // Called when window resizes to update BGFX viewport
  void onResize(uint32_t width, uint32_t height);

  // Getters
  SDL_Window* GetWindow() const;
  SDL_Texture* GetGameTexture();
  std::string GetCurrentGameContent();

  // Determine which framebuffer to use for rendering
  bgfx::FrameBufferHandle GetSceneFramebuffer() const {
    return scene_framebuffer_;
  }
  // Scene framebuffer and texture handles
  bgfx::TextureHandle GetSceneColorTexture() const {
    return scene_color_texture_;
  }
  // Reflection framebuffer and texture handles
  bgfx::FrameBufferHandle GetReflectionFramebuffer() const {
    return reflectionFB;
  }
  bgfx::TextureHandle GetReflectionColorTex() const {
    return reflectionColorTex;
  }
  uint16_t GetFramebufferWidth() const { return last_framebuffer_width_; }
  uint16_t GetFramebufferHeight() const { return last_framebuffer_height_; }

  ImTextureID GetSceneTexId() const { return scene_tex_id_; }

  // This allows an external system, like a rendering pipeline, to
  // tell the main renderer which texture should be displayed in the UI.
  void SetSceneTexture(bgfx::TextureHandle handle);

private:
  SDL_Window* window_;
  mutable std::mutex canvas_mutex_;
  SDL_Texture* game_texture_;  // Texture for the game canvas

  // BGFX framebuffer handles
  // bgfx::FrameBufferHandle scene_framebuffer_ = BGFX_INVALID_HANDLE;
  // bgfx::TextureHandle scene_color_texture_;
  int last_framebuffer_width_ = -1;
  int last_framebuffer_height_ = -1;
  uint32_t last_resize_frame_ = 0;
  const uint32_t resize_throttle_ = 5;

  // game FBO
  bgfx::FrameBufferHandle scene_framebuffer_ = BGFX_INVALID_HANDLE;
  bgfx::TextureHandle scene_color_texture_ = BGFX_INVALID_HANDLE;
  bgfx::TextureHandle scene_depth_texture_ = BGFX_INVALID_HANDLE;

  // Reflection FBO
  bgfx::FrameBufferHandle reflectionFB = BGFX_INVALID_HANDLE;
  bgfx::TextureHandle reflectionColorTex = BGFX_INVALID_HANDLE;

  ImTextureID scene_tex_id_{0};

  std::string current_game_content_;

  // Default position for rendering
  int pos_x_ = 0;
  int pos_y_ = 0;

  // Frame counter
  uint32_t frame_count_ = 0;
};

#endif  // RENDERER_GRAPHICS_H
