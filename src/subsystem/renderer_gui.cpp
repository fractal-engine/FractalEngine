#include "subsystem/renderer_gui.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstdlib>
#include <iostream>

#include <sstream>
#include <string>
#include "base/logger.h"

RendererGUI::RendererGUI()
    : window_(nullptr), sdl_renderer_(nullptr), game_texture_(nullptr) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    Logger::getInstance().Log(
        LogLevel::ERROR, std::string("SDL_Init failed: ") + SDL_GetError());
    std::exit(1);
  }

  // Initialize SDL_ttf
  if (!TTF_Init()) {
    Logger::getInstance().Log(
        LogLevel::ERROR, "TTF_Init failed: " + std::string(SDL_GetError()));
    std::exit(1);
  }

  /* SDL window: title, centered position, resizable. */
  window_ = SDL_CreateWindow("Fractal Engine", 1280, 720, SDL_WINDOW_RESIZABLE);

  if (!window_) {
    Logger::getInstance().Log(
        LogLevel::ERROR,
        std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    SDL_Quit();
    std::exit(1);
  }

  sdl_renderer_ = SDL_CreateRenderer(window_, nullptr);
  if (!sdl_renderer_) {
    Logger::getInstance().Log(
        LogLevel::ERROR,
        std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
    SDL_DestroyWindow(window_);
    SDL_Quit();
    std::exit(1);
  }

  // Set initial size in RendererBase
  SetSize(800, 600);

  // Create an off-screen texture for rendering the game contents.
  // Using SDL_TEXTUREACCESS_TARGET so we can render to it.
  game_texture_ = SDL_CreateTexture(sdl_renderer_, SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_TARGET, 800, 600);

  // Set texture blend mode to blend, removes black background during render
  SDL_SetTextureBlendMode(game_texture_, SDL_BLENDMODE_BLEND);

  if (!game_texture_) {
    Logger::getInstance().Log(
        LogLevel::ERROR,
        std::string("SDL_CreateTexture failed: ") + SDL_GetError());
  }
  Logger::getInstance().Log(LogLevel::INFO,
                            "RendererGUI initialized successfully with SDL.");
}

RendererGUI::~RendererGUI() {
  // The Logger is already destruct (The sequence of destruction is not fixed)
  // Logger::getInstance().Log(LogLevel::INFO, "RendererGUI destructor called");
  if (sdl_renderer_) {
    SDL_DestroyRenderer(sdl_renderer_);
  }
  if (window_) {
    SDL_DestroyWindow(window_);
  }
  SDL_Quit();
}

// Store single-line text as shared game content.
void RendererGUI::ShowText(const std::string& text, int x, int y) {}

/* Convert vector of text lines into single multi-line string
and store it as shared game content for rendering. */
void RendererGUI::ShowText(const std::vector<std::string>& text_area, int x,
                           int y) {}

/*
std::string RendererGUI::GetCurrentGameContent() {
  std::lock_guard<std::mutex> lock(canvas_mutex_);
  return current_game_content_;
}*/

void RendererGUI::RenderGameContent() {
  // Render to off-screen texture
  SDL_SetRenderTarget(sdl_renderer_, game_texture_);
  SDL_SetRenderDrawColor(sdl_renderer_, 0, 0, 0, 0);  // Transparent
  SDL_RenderClear(sdl_renderer_);                     // Clear texture

  // Render game content here

  // Reset render target to default window
  SDL_SetRenderTarget(sdl_renderer_, NULL);
}

void RendererGUI::ClearDisplay() {
  // std::lock_guard<std::mutex> lock(canvas_mutex_);
  SDL_SetRenderDrawColor(sdl_renderer_, 30, 30, 30, 255);
  SDL_RenderClear(sdl_renderer_);
}

void RendererGUI::Render() {
  // Present drawn frame
  SDL_RenderPresent(sdl_renderer_);
  // Signal that frame has been redrawn
  redrawn();
}

void RendererGUI::ProcessEvents(bool& quit) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) {
      quit = true;
    }
  }
}

// Getters for the SDL window and renderer.
SDL_Window* RendererGUI::GetWindow() const {
  return window_;
}

SDL_Renderer* RendererGUI::GetSDLRenderer() const {
  return sdl_renderer_;
}

SDL_Texture* RendererGUI::GetGameTexture() {
  return game_texture_;
}
