#include "renderer/renderer_graphics.h"

#include <SDL.h>
#include <SDL2/SDL_ttf.h>

#include <backends/imgui_impl_sdl2.h>
#include <imgui.h>

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#include "core/engine_globals.h"
#include "core/logger.h"
#include "core/view_ids.h"

#include "renderer/shaders/shader_manager.h"
#include "renderer/shaders/shader_utils.h"

#include "subsystem/subsystem_manager.h"

#include "platform/platform_utils.h"

GraphicsRenderer::GraphicsRenderer() {
  // Initialize SDL_ttf (SDL itself is initialized by window manager)
  if (TTF_Init() == -1) {
    Logger::getInstance().Log(
        LogLevel::Error, "TTF_Init failed: " + std::string(TTF_GetError()));
    std::exit(1);
  }

  // Get SDL window from WindowManager instead of creating one
  window_ = WindowManager::GetWindow();
  if (!window_) {
    Logger::getInstance().Log(LogLevel::Error,
                              "Failed to get window from WindowManager");
    std::exit(1);
  }

  // Set initial size from WindowManager
  // SetSize(WindowManager::GetWidth(), WindowManager::GetHeight());

  // Register for window resize notifications
  WindowManager::RegisterResizeCallback(
      [this](int width, int height) { SetSize(width, height); });

  Logger::getInstance().Log(
      LogLevel::Info, "GraphicsRenderer initialized successfully with BGFX.");
}

GraphicsRenderer::~GraphicsRenderer() {
  TTF_Quit();
}

// Initialize BGFX with platform data
bool GraphicsRenderer::InitBGFX() {
  bgfx::Init init;
  platform::SetupBGFXPlatformData(init, window_);

  init.type = bgfx::RendererType::Count;  // Auto-select backend
  init.debug = true;
  init.profile = true;

  if (!bgfx::init(init)) {
    Logger::getInstance().Log(LogLevel::Error, "Failed to initialize BGFX!");
    return false;
  }

  // Initialize shaders
  InitShaders();

  auto backend = bgfx::getRendererType();
  Logger::getInstance().Log(
      LogLevel::Info, "BGFX initialized successfully! Selected backend: " +
                          std::to_string(static_cast<int>(backend)) + " (" +
                          bgfx::getRendererName(backend) + ")");
  return true;
}

// Store single-line text as shared game content.
void GraphicsRenderer::ShowText(const std::string& text, int x, int y) {
  std::lock_guard<std::mutex> lock(canvas_mutex_);
  current_game_content_ = text;
  pos_x_ = x;
  pos_y_ = y;
}

/* Convert vector of text lines into single multi-line string
and store it as shared game content for rendering. */
void GraphicsRenderer::ShowText(const std::vector<std::string>& text_area,
                                int x, int y) {
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

std::string GraphicsRenderer::GetCurrentGameContent() {
  std::lock_guard<std::mutex> lock(canvas_mutex_);
  return current_game_content_;
}

// Handle view setup and framebuffer creation
void GraphicsRenderer::ConfigureViews() {
  int fbw, fbh;
  platform::GetDrawableSize(window_, &fbw, &fbh);

  // Only recreate framebuffers if resolution changed
  if (fbw != lastFramebufferWidth_ || fbh != lastFramebufferHeight_) {
    CreateFramebuffers(fbw, fbh);
    lastFramebufferWidth_ = fbw;
    lastFramebufferHeight_ = fbh;
  }

  // Game view
  bgfx::setViewClear(ViewID::GAME, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                     0x1e1e1eff, 1.0f, 0);
  bgfx::setViewRect(ViewID::GAME, 0, 0, fbw, fbh);
  bgfx::setViewFrameBuffer(ViewID::GAME, gameFramebuffer_);
  bgfx::touch(ViewID::GAME);

  // UI background view
  bgfx::setViewClear(ViewID::UI_BACKGROUND, BGFX_CLEAR_COLOR, 0x1e1e1eff, 1.0f,
                     0);
  bgfx::setViewRect(ViewID::UI_BACKGROUND, 0, 0, fbw, fbh);
  bgfx::touch(ViewID::UI_BACKGROUND);
}

// Set up per-frame and clear operations
void GraphicsRenderer::PrepareFrame() {
  int fbw, fbh;
  platform::GetDrawableSize(window_, &fbw, &fbh);

  bgfx::setViewClear(ViewID::CLEAR, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                     0x2d2d2dff, 1.0f, 0);
  bgfx::setViewRect(ViewID::CLEAR, 0, 0, fbw, fbh);
}

// Framebuffer initialization and handling
void GraphicsRenderer::CreateFramebuffers(int width, int height) {
  // add new framebuffers  here

  if (bgfx::isValid(gameFramebuffer_)) {
    bgfx::destroy(gameFramebuffer_);
  }
}

void GraphicsRenderer::Render() {
  // view and projection matrices can be added here if needed

  // Render game objects here

  bgfx::touch(
      ViewID::CLEAR);  // mark the view as used even if nothing is submitted

  // Submit the entire frame
  // bgfx::frame();

  bgfx::touch(ViewID::GAME);

  // Signal that a frame was rendered
  redrawn();
  frameCount_++;
}

void GraphicsRenderer::ProcessEvents(bool& quit) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      quit = true;
    }
  }

  // window size check
  int w, h;
  SDL_GetWindowSize(WindowManager::GetWindow(), &w, &h);
  if (w != WindowManager::GetWidth() || h != WindowManager::GetHeight()) {
    WindowManager::OnWindowResize(w, h);  // Will trigger the BGFX resize logic
  }
}

void GraphicsRenderer::ClearDisplay() {
  // clear the BGFX view with nothing drawn
  bgfx::touch(0);
}

void GraphicsRenderer::Shutdown() {
  Logger::getInstance().Log(LogLevel::Info, "Shutting down GraphicsRenderer");

  if (bgfx::isValid(gameFramebuffer_))
    bgfx::destroy(gameFramebuffer_);

  bgfx::frame();
  bgfx::shutdown();
}

void GraphicsRenderer::BeginImGuiFrame() {
  imgui_backend_.BeginFrame();
}

void GraphicsRenderer::EndImGuiFrame() {
  imgui_backend_.EndFrame();
}

// function to initialize shaders
void GraphicsRenderer::InitShaders() {}

// Getters for the SDL window and renderer.
SDL_Window* GraphicsRenderer::GetWindow() const {
  return window_;
}