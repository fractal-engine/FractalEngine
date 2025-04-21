#include "window_manager.h"
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include "base/logger.h"

#include "platform/platform_utils.h"

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
    Logger::getInstance().Log(
        LogLevel::Error,
        std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    SDL_Quit();
    return false;
  }

  SDL_RaiseWindow(instance.window_);

  // ------------------------------------------------
  //  Get display scale factor for Retina support
  // ------------------------------------------------
  instance.dpiScale_ = platform::GetDPIScale(instance.window_);

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
  platform::SetupBGFXPlatformData(init, getInstance().window_);
}
