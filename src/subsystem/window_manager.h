#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include <SDL.h>
#include <SDL_hints.h>
#include <SDL_syswm.h> 
#include <bgfx/bgfx.h>

#include <functional>
#include <vector>
#include "base/singleton.hpp"

class WindowManager : public Singleton<WindowManager> {
private:
  SDL_Window* window_;
  int width_;
  int height_;
  std::vector<std::function<void(int, int)>> resizeCallbacks_;

  void initialize();

public:
  WindowManager() : window_(nullptr), width_(1280), height_(720) {}
  ~WindowManager();

  // Prevent copying
  WindowManager(const WindowManager&) = delete;
  WindowManager& operator=(const WindowManager&) = delete;

  // Initialize with window management
  static bool Initialize(const char* title = "Fractal Engine", int width = 1280,
                         int height = 720);
  static void Shutdown();

  // Window properties
  static int GetWidth();
  static int GetHeight();
  static SDL_Window* GetWindow();

  // Register for resize notifications
  static void RegisterResizeCallback(std::function<void(int, int)> callback);

  // Handle window resize
  static void OnWindowResize(int width, int height);

  // Initialize BGFX platform data
  static void InitBGFXPlatformData(bgfx::Init& init);
};

#endif  // WINDOW_MANAGER_H