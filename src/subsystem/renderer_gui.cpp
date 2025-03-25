#include "subsystem/renderer_gui.h"

#include <SDL.h>
#include <SDL2/SDL_ttf.h>

#include <cstdlib>
#include <iostream>

#include <sstream>
#include <string>
#include "base/logger.h"

RendererGUI::RendererGUI()
    : window_(nullptr), sdl_renderer_(nullptr), game_texture_(nullptr) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    Logger::getInstance().Log(
        LogLevel::ERROR, std::string("SDL_Init failed: ") + SDL_GetError());
    std::exit(1);
  }

  // Initialize SDL_ttf
  if (TTF_Init() == -1) {
    Logger::getInstance().Log(
        LogLevel::ERROR, "TTF_Init failed: " + std::string(TTF_GetError()));
    std::exit(1);
  }

  /* SDL window: title, centered position, resizable. */
  window_ = SDL_CreateWindow("Fractal Engine", SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED, 1280, 720,  // 1280x720
                             SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!window_) {
    Logger::getInstance().Log(
        LogLevel::ERROR,
        std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    SDL_Quit();
    std::exit(1);
  }
  sdl_renderer_ = SDL_CreateRenderer(
      window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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
void RendererGUI::ShowText(const std::string& text, int x, int y) {
  std::lock_guard<std::mutex> lock(canvas_mutex_);
  current_game_content_ = text;
  pos_x_ = x; 
  pos_y_ = y; 
}

/* Convert vector of text lines into single multi-line string
and store it as shared game content for rendering. */
void RendererGUI::ShowText(const std::vector<std::string>& text_area, int x,
                           int y) {
  if (!text_area.empty()) {
    std::string combined;
    for (const auto& line : text_area) {
      combined += line + "\n";  // Append newline to combined string
    }
    std::lock_guard<std::mutex> lock(canvas_mutex_);
    current_game_content_ = combined;  // Store combined string
    pos_x_ = x;
    pos_y_ = y;
  }
}

std::string RendererGUI::GetCurrentGameContent() {
  std::lock_guard<std::mutex> lock(canvas_mutex_);
  return current_game_content_;
}

void RendererGUI::RenderGameContent() {
  // Render to off-screen texture
  SDL_SetRenderTarget(sdl_renderer_, game_texture_);
  SDL_SetRenderDrawColor(sdl_renderer_, 0, 0, 0, 0);  // Transparent
  SDL_RenderClear(sdl_renderer_);                     // Clear texture

  // Retrieve the current game content
  std::string asciiArt = GetCurrentGameContent();

  // Debug - show game content in log
  // Logger::getInstance().Log(LogLevel::DEBUG, "ASCII Art: " + asciiArt);

  // fixed-width font
  TTF_Font* font = TTF_OpenFont("NotoSansMono_Regular.ttf", 12);
  if (!font) {
    Logger::getInstance().Log(
        LogLevel::ERROR, "TTF_OpenFont failed: " + std::string(TTF_GetError()));
    SDL_SetRenderTarget(sdl_renderer_, NULL);  // handle error
    return;
  }

  // Split ASCII into lines
  std::vector<std::string> lines;
  {
    std::istringstream iss(asciiArt);
    for (std::string line; std::getline(iss, line);) {
      lines.push_back(line);
    }
  }

  SDL_Color textColor = {255, 255, 255, 255};
  int yPos = 0;  // Where we draw the next line
  for (auto& line : lines) {
    if (line.empty()) {
      // Move down by a small skip if the line is blank
      yPos += TTF_FontLineSkip(font);
      continue;
    }

    // Render this single line
    SDL_Surface* surf = TTF_RenderText_Blended(font, line.c_str(), textColor);
    if (!surf) {
      // handle error
      continue;
    }

    SDL_Texture* lineTexture =
        SDL_CreateTextureFromSurface(sdl_renderer_, surf);
    if (lineTexture) {
      SDL_Rect destRect{ pos_x_, yPos + pos_y_, surf->w, surf->h };
      SDL_RenderCopy(sdl_renderer_, lineTexture, NULL, &destRect);
      SDL_DestroyTexture(lineTexture);
    }

    // Move down by the line's height or by TTF_FontLineSkip if you prefer
    yPos += surf->h;
    SDL_FreeSurface(surf);
  }

  TTF_CloseFont(font);

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
    if (event.type == SDL_QUIT) {
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
