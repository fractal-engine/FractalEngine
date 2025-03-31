#include <chrono>
#include <ftxui/component/component.hpp>  // For Renderer, ScreenInteractive
#include <ftxui/component/screen_interactive.hpp>  // For ScreenInteractive::Fullscreen
#include <ftxui/dom/elements.hpp>  // For elements such as text, vbox, hbox, etc.
#include <ftxui/screen/screen.hpp>  // For Screen
#include <iostream>
#include <memory>
#include <thread>
#include "audio/sound_manager.h"  // For SoundManager
#include "base/game_base.h"
#include "base/logger.h"
#include "game/game_test.h"
#include "subsystem/subsystem_manager.h"

int main() {
  // Initialize the subsystem manager
  SubsystemManager::Initialize();

  // Initialize BGFX
  Logger::getInstance().Log(LogLevel::DEBUG, "Initializing BGFX...");
  init.type = bgfx::RendererType::Direct3D11;  // For Windows with DirectX 11
  init.type = bgfx::RendererType::Vulkan;
  bgfx::Init init;
  init.type =
      bgfx::RendererType::Count;  // Automatically determine renderer type
  init.resolution.width = 1280;   // Screen width
  init.resolution.height = 720;   // Screen height
  init.resolution.reset = BGFX_RESET_VSYNC;

  if (!bgfx::init(init)) {
    Logger::getInstance().Log(LogLevel::ERROR, "Failed to initialize BGFX!");
    return -1;  // Exit on failure
  }

  // Configure the default view (view ID 0) to clear with a color and depth
  // buffer
  bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000FF, 1.0f,
                     0);
  bgfx::setViewRect(0, 0, 0, 1280, 720);

  Logger::getInstance().Log(LogLevel::DEBUG, "BGFX initialized successfully!");

  // Start the game manager in a separate thread
  std::thread game_thread([&] { SubsystemManager::GetGameManager()->Run(); });

  // Log the start of the editor display
  Logger::getInstance().Log(LogLevel::DEBUG, "Initializing editor display");

  // Initialize the sound system
  if (!SoundManager::Instance().init()) {
    Logger::getInstance().Log(LogLevel::ERROR,
                              "Failed to initialize SoundManager");
    return -1;
  }

  // Optionally adjust ambient volume (e.g., to 10%)
  SoundManager::Instance().setAmbientVolume(0.1f);

  // Start the ambient background sound
  if (!SoundManager::Instance().startAmbient()) {
    Logger::getInstance().Log(LogLevel::ERROR, "Failed to start ambient sound");
  }

  // Run the editor
  SubsystemManager::GetEditor()->Run();

  // Join the game thread
  game_thread.join();

  // Log the joining of the game thread
  Logger::getInstance().Log(LogLevel::INFO, "Game thread joined");

  // Shutdown BGFX before exiting
  Logger::getInstance().Log(LogLevel::DEBUG, "Shutting down BGFX...");
  bgfx::shutdown();

  // Clean up the sound system on exit
  SoundManager::Instance().terminate();

  return 0;
}