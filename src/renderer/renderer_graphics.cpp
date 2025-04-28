#include "renderer/renderer_graphics.h"

#include <SDL.h>
#include <SDL2/SDL_ttf.h>

#include <backends/imgui_impl_sdl2.h>
#include <imgui.h>

#include <bx/math.h>
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

  bgfx::setDebug(BGFX_DEBUG_TEXT | BGFX_DEBUG_STATS);

  // Initialize shaders
  InitShaders();

  ConfigureViews();

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
  // 1. Called once on init and in WindowManager::OnWindowResize()
  int fbw, fbh;
  platform::GetDrawableSize(window_, &fbw, &fbh);

  // Create/resize off-screen FBO only
  if (fbw != lastFramebufferWidth_ || fbh != lastFramebufferHeight_)
    CreateFramebuffers(fbw, fbh);

  // UI background view lives in back-buffer, full window size
  bgfx::setViewClear(ViewID::UI_BACKGROUND, BGFX_CLEAR_COLOR, 0x1e1e1eff, 1.0f,
                     0);
  bgfx::setViewRect(ViewID::UI_BACKGROUND, 0, 0, fbw, fbh);
}

void GraphicsRenderer::SetSize(int w, int h) {
  lastFramebufferWidth_ = 0;  // force rebuild on next PrepareFrame()
  lastFramebufferHeight_ = 0;
  ConfigureViews();  // rebuild views for new window
}

// Set up per-frame and clear operations
void GraphicsRenderer::PrepareFrame() {
  // Get current viewport dimensions (from ImGui canvas)
  const uint16_t fbw = canvasViewportW ? canvasViewportW : 1;
  const uint16_t fbh = canvasViewportH ? canvasViewportH : 1;

  // Create/recreate framebuffers if size changed
  if (fbw != lastFramebufferWidth_ || fbh != lastFramebufferHeight_) {
    CreateFramebuffers(fbw, fbh);
  }

  // set up scene view with clear values and framebuffer
  bgfx::setViewClear(ViewID::SCENE, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                     0x303030ff, 1.0f, 0);
  bgfx::setViewRect(ViewID::SCENE, 0, 0, fbw, fbh);
  bgfx::setViewFrameBuffer(ViewID::SCENE, scene_framebuffer_);

  // debug
  if (!bgfx::isValid(scene_framebuffer_)) {
    Logger::getInstance().Log(LogLevel::Error,
                              "Scene framebuffer is invalid in PrepareFrame!");
  }
}

// Framebuffer initialization and handling
void GraphicsRenderer::CreateFramebuffers(uint16_t w, uint16_t h) {
  // 1. If an old FBO exists, finish the *current* frame first
  if (bgfx::isValid(scene_framebuffer_)) {
    bgfx::frame();                      // flush everything that still
                                        //  uses scene_framebuffer_
    bgfx::destroy(scene_framebuffer_);
    scene_framebuffer_ = BGFX_INVALID_HANDLE;
  }

  // 2. Create NEW colour & depth RT (no sRGB flag)
  scene_color_texture_ = bgfx::createTexture2D(
      w, h, false, 1, bgfx::TextureFormat::BGRA8,
      BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);

  scene_depth_texture_ =
      bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::D24S8,
                            BGFX_TEXTURE_RT | BGFX_TEXTURE_RT_WRITE_ONLY);

  const bgfx::TextureHandle attachments[2] = {scene_color_texture_,
                                              scene_depth_texture_};
  scene_framebuffer_ = bgfx::createFrameBuffer(
      BX_COUNTOF(attachments), attachments, /*destroyTextures*/ true);

  // 3. Pass handle to ImGui via helper overload (no raw cast)
  scene_tex_id_ = (ImTextureID)(uintptr_t)scene_color_texture_.idx;
  Logger::getInstance().Log(
      LogLevel::Debug,
      "[FBO] new colour RT idx = " + std::to_string(scene_color_texture_.idx));

  lastFramebufferWidth_ = w;
  lastFramebufferHeight_ = h;
}

void GraphicsRenderer::Render() {

  bgfx::touch(ViewID::UI_BACKGROUND);  // dummy background view
  // bgfx::touch(ViewID::UI);             // ImGui rendering into UI view
  // bgfx::touch(ViewID::SCENE);

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

  if (bgfx::isValid(scene_framebuffer_))
    bgfx::destroy(scene_framebuffer_);
  scene_tex_id_ = 0;

  bgfx::frame();
  bgfx::shutdown();
}

void GraphicsRenderer::BeginImGuiFrame() {}

void GraphicsRenderer::EndImGuiFrame() {}

// function to initialize shaders
void GraphicsRenderer::InitShaders() {

  // Fullscreen quad vertices
  struct FSVertex {
    float x, y, z;
    float u, v;
  };

  // use triangle strip topology
  static FSVertex quadVertices[] = {
      // Triangle strip order: TL -> TR -> BL -> BR
      {-1.0f, 1.0f, 0.0f, 0.0f, 1.0f},   // Top-left
      {1.0f, 1.0f, 0.0f, 1.0f, 1.0f},    // Top-right
      {-1.0f, -1.0f, 0.0f, 0.0f, 0.0f},  // Bottom-left
      {1.0f, -1.0f, 0.0f, 1.0f, 0.0f},   // Bottom-right
  };

  // Submit as triangle strip
  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA |
                 BGFX_STATE_PT_TRISTRIP);
}

// Getters for the SDL window and renderer.
SDL_Window* GraphicsRenderer::GetWindow() const {
  return window_;
}