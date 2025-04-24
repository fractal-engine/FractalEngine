#ifndef renderer_graphics_H
#define renderer_graphics_H

#include <SDL.h>

#include <mutex>
#include <string>
#include <vector>

#include "renderer/renderer_base.h"

#include "drivers/imgui_backend.h"

#include "subsystem/window_manager.h"

class GraphicsRenderer : public RendererBase {
public:
  // The constructor takes a mutex reference
  GraphicsRenderer();
  virtual ~GraphicsRenderer();  // Virtual destructor

  // Initializes BGFX
  bool InitBGFX();

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
  void CreateFramebuffers(int width, int height);

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

private:
  SDL_Window* window_;
  mutable std::mutex canvas_mutex_;
  SDL_Texture* game_texture_;  // Texture for the game canvas

  // BGFX framebuffer handles
  bgfx::FrameBufferHandle gameFramebuffer_ = BGFX_INVALID_HANDLE;
  int lastFramebufferWidth_ = -1;
  int lastFramebufferHeight_ = -1;

  std::string current_game_content_;

  // Default position for rendering
  int pos_x_ = 0;
  int pos_y_ = 0;

  ImGuiBackend imgui_backend_;

  // Frame counter
  uint32_t frameCount_ = 0;
};

#endif  // renderer_graphics_H
