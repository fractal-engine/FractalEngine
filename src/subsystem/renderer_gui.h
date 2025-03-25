#ifndef RENDERER_GUI_H
#define RENDERER_GUI_H

#include <SDL.h>
#include <mutex>
#include <string>
#include <vector>

#include "base/renderer_base.h"

class RendererGUI : public RendererBase {
public:
  // The constructor takes a mutex reference
  RendererGUI();
  virtual ~RendererGUI(); // Virtual destructor

  // RendererBase interface
  void ClearDisplay() override;
  void ShowText(const std::string& text, int x, int y) override;
  void ShowText(const std::vector<std::string>& text_area, int x,
                int y) override;

  void RenderGameContent();

  // Called after drawing to present the frame.
  void Render();

  // Processes SDL events, including quitting – called from the Editor loop.
  void ProcessEvents(bool& quit);

  // Getters
  SDL_Window* GetWindow() const;
  SDL_Renderer* GetSDLRenderer() const;
  SDL_Texture* GetGameTexture();
  std::string GetCurrentGameContent();

private:
  SDL_Window* window_;
  SDL_Renderer* sdl_renderer_;
  mutable std::mutex canvas_mutex_;
  SDL_Texture* game_texture_;  // Texture for the game canvas

  std::string current_game_content_;

  // Default position for rendering
  int pos_x_ = 0;
  int pos_y_ = 0;
};

#endif  // RENDERER_GUI_H
