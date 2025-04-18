#include "window_manager.h"
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include "base/logger.h"

#ifdef __APPLE__
#include "platform/bgfx_macos.h"
#include "platform/window_macos.h"
#endif

bool WindowManager::Initialize(const char* title, int width, int height) {
  WindowManager& instance = getInstance();
  instance.width_ = width;
  instance.height_ = height;

  Logger::getInstance().Log(LogLevel::Debug,
                            "Calling SDL_Init");  // debug - remove later

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    Logger::getInstance().Log(
        LogLevel::Error, std::string("SDL_Init failed: ") + SDL_GetError());
    return false;
  }

  instance.window_ = SDL_CreateWindow(
      title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  if (!instance.window_) {

    // Make sure window is raised
    SDL_RaiseWindow(instance.window_);

    Logger::getInstance().Log(
        LogLevel::Error,
        std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    SDL_Quit();
    return false;
  }

// ------------------------------------------------
//  Get macOS display scale factor for Retina support
// ------------------------------------------------
#ifdef __APPLE__
  instance.dpiScale_ = WindowManager_GetDPIScale(instance.window_);
#endif

  Logger::getInstance().Log(
      LogLevel::Info, "WindowManager initialized with size: " +
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
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
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

float WindowManager::GetDPIScale() {
  return getInstance().dpiScale_;
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
      LogLevel::Debug, "Window resized to: " + std::to_string(width) + "x" +
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
  bgfx_macos::SetupPlatformData(init, getInstance().window_);

#elif defined(_WIN32)  // TODO: move code to bgfx_win32.cpp
  SDL_SysWMinfo wmi;
  SDL_VERSION(&wmi.version);
  if (SDL_GetWindowWMInfo(getInstance().window_, &wmi)) {
    init.platformData.ndt = nullptr;
    init.platformData.nwh = wmi.info.win.window;
    init.platformData.context = nullptr;
    Logger::getInstance().Log(LogLevel::Debug,
                              "Set BGFX platformData for Windows.");
  }

#elif defined(__linux__)  // TODO: move code to bgfx_linux.cpp
  SDL_SysWMinfo wmi;
  SDL_VERSION(&wmi.version);
  if (SDL_GetWindowWMInfo(getInstance().window_, &wmi)) {
    init.platformData.ndt = wmi.info.x11.display;
    init.platformData.nwh = (void*)(uintptr_t)wmi.info.x11.window;
    init.platformData.context = nullptr;
    Logger::getInstance().Log(LogLevel::Debug,
                              "Set BGFX platformData for Linux (X11).");
  }
#endif
}
