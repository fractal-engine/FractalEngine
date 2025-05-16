#include <bgfx/bgfx.h>
#include <chrono>
#include <ftxui/component/component.hpp>  // For Renderer, ScreenInteractive
#include <ftxui/component/screen_interactive.hpp>  // For ScreenInteractive::Fullscreen
#include <ftxui/dom/elements.hpp>  // For elements such as text, vbox, hbox, etc.
#include <ftxui/screen/screen.hpp>  // For Screen
#include <iostream>
#include <memory>
#include <thread>
#include "audio/sound_manager.h"  // For SoundManager
#include "core/logger.h"
#include "game/game_base.h"
#include "game/game_test.h"
#include "renderer/shaders/shader_utils.h"
#include "renderer/shaders/shader_manager.h"
#include "subsystem/subsystem_manager.h"

int main() {
  Logger::getInstance().Log(LogLevel::Info, "Starting Fractal Engine");

  // Initialize the subsystem manager
  SubsystemManager::Initialize();

  // Start the game manager in a separate thread
  std::thread game_thread([&] { SubsystemManager::GetGameManager()->Run(); });

  // Log the start of the editor display
  Logger::getInstance().Log(LogLevel::Debug, "Initializing editor display");

  // Initialize the sound system
  if (!SoundManager::Instance().init()) {
    Logger::getInstance().Log(LogLevel::Error,
                              "Failed to initialize SoundManager");
    return -1;
  }

  // Optionally adjust ambient volume (e.g., to 10%)
  SoundManager::Instance().setAmbientVolume(0.1f);  // TODO: change this value

  // Start the ambient background sound
  if (!SoundManager::Instance().startAmbient()) {
    Logger::getInstance().Log(LogLevel::Error, "Failed to start ambient sound");
  }

  // Run the editor
  Logger::getInstance().Log(LogLevel::Info, "Calling EditorGUI::Run()");
  SubsystemManager::GetEditor()->Run();
  Logger::getInstance().Log(LogLevel::Info,
                            "Returned from EditorGUI::Run()");  // debug

  // Join the game thread
  game_thread.join();

  // Log the joining of the game thread
  Logger::getInstance().Log(LogLevel::Info, "Game thread joined");

  // Shutdown subsystems
  SubsystemManager::Shutdown();
  Logger::getInstance().Log(LogLevel::Debug,
                            "SubsystemManager shutdown started in main");

  // Clean up the sound system on exit
  SoundManager::Instance().terminate();

  return 0;
}