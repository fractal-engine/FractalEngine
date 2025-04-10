#include "subsystem/graphics_renderer.h"

#include <SDL.h>
#include <SDL2/SDL_ttf.h>

#include <backends/imgui_impl_sdl2.h>
#include <imgui.h>

#include <cstdlib>
#include <iostream>

#include <sstream>
#include <string>
#include "base/logger.h"
#include "base/shader_utils.h"
#include "base/view_ids.h"

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
  CleanupShaders();  // Clean up shaders
  TTF_Quit();
}

// Initialize BGFX with platform data
bool GraphicsRenderer::InitBGFX() {
  bgfx::Init init;
  WindowManager::InitBGFXPlatformData(init);
  init.type = bgfx::RendererType::Count;  // Let BGFX auto-select
  init.debug = true;
  init.profile = true;

  if (!bgfx::init(init)) {
    Logger::getInstance().Log(LogLevel::Error, "Failed to initialize BGFX!");
    return false;
  }

  bgfx::RendererType::Enum backend = bgfx::getRendererType();
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

// Clean up shader programs
void GraphicsRenderer::CleanupShaders() {
  for (auto& pair : shaderPrograms_) {
    if (bgfx::isValid(pair.second))
      bgfx::destroy(pair.second);
  }
  shaderPrograms_.clear();
}

void GraphicsRenderer::ConfigureViews() {
  // GAME View
  bgfx::setViewClear(ViewID::GAME, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                     0x1e1e1eff, 1.0f, 0);
  bgfx::setViewRect(ViewID::GAME, 0, 0, width_, height_);
  bgfx::touch(ViewID::GAME);

  // UI_BACKGROUND View (behind ImGui)
  bgfx::setViewClear(ViewID::UI_BACKGROUND, BGFX_CLEAR_COLOR, 0x1e1e1eff, 1.0f,
                     0);  // darker than UI
  bgfx::setViewRect(ViewID::UI_BACKGROUND, 0, 0, width_, height_);
  bgfx::touch(ViewID::UI_BACKGROUND);
}

void GraphicsRenderer::PrepareFrame() {
  ConfigureViews();

  // Clear the view
  bgfx::setViewClear(ViewID::CLEAR, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                     0x2d2d2dff, 1.0f, 0);
  bgfx::setViewRect(ViewID::CLEAR, 0, 0, width_, height_);

  // debug text
  // bgfx::setDebug(BGFX_DEBUG_TEXT);
  // bgfx::dbgTextClear();
  // bgfx::dbgTextPrintf(0, 1, 0x0f, "Current frame: %d", frameCount_);
}

void GraphicsRenderer::Render() {
  // view and projection matrices can be added here if needed

  // Render game objects here

  bgfx::touch(
      ViewID::CLEAR);  // mark the view as used even if nothing is submitted

  // Submit the entire frame
  //  bgfx::frame();

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

  CleanupShaders();
  bgfx::frame();
  bgfx::shutdown();
}

void GraphicsRenderer::BeginImGuiFrame() {
  imgui_renderer_.BeginFrame();
}

void GraphicsRenderer::EndImGuiFrame() {
  imgui_renderer_.EndFrame();
}

// Utility function to load and create a shader program
bgfx::ProgramHandle GraphicsRenderer::LoadShaderProgram(
    const std::string& programName, const std::string& vertexShaderFilename,
    const std::string& fragmentShaderFilename) {

  Logger::getInstance().Log(
      LogLevel::Debug, "Looking for vertex shader: " + vertexShaderFilename);
  Logger::getInstance().Log(LogLevel::Debug, "Looking for fragment shader: " +
                                                 fragmentShaderFilename);

  // Reuse if already loaded
  if (shaderPrograms_.count(programName))
    return shaderPrograms_[programName];

  Logger::getInstance().Log(LogLevel::Info,
                            "Loading shader program: " + programName);

  bgfx::ShaderHandle vs = loadShader(vertexShaderFilename.c_str());
  bgfx::ShaderHandle fs = loadShader(fragmentShaderFilename.c_str());

  if (!bgfx::isValid(vs) || !bgfx::isValid(fs)) {
    Logger::getInstance().Log(
        LogLevel::Error, "Failed to load shaders for program: " + programName);
    return BGFX_INVALID_HANDLE;
  }

  bgfx::ProgramHandle program = bgfx::createProgram(vs, fs, true);
  shaderPrograms_[programName] = program;
  return program;
}

// Getters for the SDL window and renderer.
SDL_Window* GraphicsRenderer::GetWindow() const {
  return window_;
}