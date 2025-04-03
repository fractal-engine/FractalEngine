#include "window_manager.h"
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include "base/logger.h"

bool WindowManager::Initialize(const char* title, int width, int height) {
  WindowManager& instance = getInstance();
  instance.width_ = width;
  instance.height_ = height;

  Logger::getInstance().Log(LogLevel::DEBUG, "Calling SDL_Init"); // debug - remove later

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    Logger::getInstance().Log(
        LogLevel::ERROR, std::string("SDL_Init failed: ") + SDL_GetError());
    return false;
  }

  instance.window_ = SDL_CreateWindow(
      title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  if (!instance.window_) {

    // Make sure window is raised
    SDL_RaiseWindow(instance.window_);

    Logger::getInstance().Log(
        LogLevel::ERROR,
        std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    SDL_Quit();
    return false;
  }

  Logger::getInstance().Log(
      LogLevel::INFO, "WindowManager initialized with size: " +
                          std::to_string(width) + "x" + std::to_string(height));
  return true;
}

void WindowManager::Shutdown() {
  getInstance().~WindowManager();
}

WindowManager::~WindowManager() {
  if (window_) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }
  SDL_Quit();
}

int WindowManager::GetWidth() {
  return getInstance().width_;
}

int WindowManager::GetHeight() {
  return getInstance().height_;
}

SDL_Window* WindowManager::GetWindow() {
  return getInstance().window_;
}

void WindowManager::RegisterResizeCallback(
    std::function<void(int, int)> callback) {
  getInstance().resizeCallbacks_.push_back(callback);
}

void WindowManager::OnWindowResize(int width, int height) {
  WindowManager& instance = getInstance();
  instance.width_ = width;
  instance.height_ = height;

  Logger::getInstance().Log(
      LogLevel::DEBUG, "Window resized to: " + std::to_string(width) + "x" +
                           std::to_string(height));

  // Notify all registered callbacks
  for (auto& callback : instance.resizeCallbacks_) {
    callback(width, height);
  }
}

void WindowManager::InitBGFXPlatformData(bgfx::Init& init) {
  int w, h;
  SDL_GetWindowSize(getInstance().window_, &w, &h);
  getInstance().width_ = w;
  getInstance().height_ = h;

  init.resolution.width = w;
  init.resolution.height = h;
  init.resolution.reset = BGFX_RESET_VSYNC;

#if defined(__APPLE__)
  SDL_SysWMinfo wmi;
  SDL_VERSION(&wmi.version);
  if (SDL_GetWindowWMInfo(getInstance().window_, &wmi)) {
    init.platformData.ndt = nullptr;
    init.platformData.nwh = wmi.info.cocoa.window;  // required for OpenGL
    init.platformData.context = nullptr;

    Logger::getInstance().Log(
        LogLevel::DEBUG,
        "Set BGFX platformData for macOS (OpenGL). nwh = " +
            std::to_string(reinterpret_cast<uintptr_t>(init.platformData.nwh)));
  } else {
    Logger::getInstance().Log(LogLevel::ERROR, "SDL_GetWindowWMInfo failed!");
  }

#elif defined(_WIN32)
  SDL_SysWMinfo wmi;
  SDL_VERSION(&wmi.version);
  if (SDL_GetWindowWMInfo(getInstance().window_, &wmi)) {
    init.platformData.ndt = nullptr;
    init.platformData.nwh = wmi.info.win.window;
    init.platformData.context = nullptr;
    Logger::getInstance().Log(LogLevel::DEBUG,
                              "Set BGFX platformData for Windows.");
  }

#elif defined(__linux__)
  SDL_SysWMinfo wmi;
  SDL_VERSION(&wmi.version);
  if (SDL_GetWindowWMInfo(getInstance().window_, &wmi)) {
    init.platformData.ndt = wmi.info.x11.display;
    init.platformData.nwh = (void*)(uintptr_t)wmi.info.x11.window;
    init.platformData.context = nullptr;
    Logger::getInstance().Log(LogLevel::DEBUG,
                              "Set BGFX platformData for Linux (X11).");
  }
#endif
}
