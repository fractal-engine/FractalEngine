#include <bgfx/bgfx.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include "editor/runtime/application.h"
#include "engine/audio/sound_manager.h"
#include "engine/core/logger.h"
#include "engine/renderer/shaders/shader_manager.h"
#include "engine/resources/shader_utils.h"
#include "game/game_base.h"
#include "game/game_test.h"

int main() {
  Logger::getInstance().Log(LogLevel::Info, "Starting Fractal Engine");

  // Initialize the subsystem manager
  Application::Initialize();

  // Start the game manager in a separate thread
  std::thread game_thread([&] { Application::GetGameManager()->Run(); });

  // Log the start of the editor display
  Logger::getInstance().Log(LogLevel::Debug, "Initializing editor display");

  // Initialize the sound system
  if (!SoundManager::Instance().init()) {
    Logger::getInstance().Log(LogLevel::Error,
                              "Failed to initialize SoundManager");
    return -1;
  }

  // Optionally adjust ambient volume (e.g., to 10%)
  SoundManager::Instance().setAmbientVolume(0.7f);  // TODO: change this value

  // Start the ambient background sound
  if (!SoundManager::Instance().startAmbient()) {
    Logger::getInstance().Log(LogLevel::Error, "Failed to start ambient sound");
  }

  // Run the editor
  Logger::getInstance().Log(LogLevel::Info, "Calling EditorGUI::Run()");
  Application::GetEditor()->Run();
  Logger::getInstance().Log(LogLevel::Info,
                            "Returned from EditorGUI::Run()");  // debug

  // Join the game thread
  game_thread.join();

  // Log the joining of the game thread
  Logger::getInstance().Log(LogLevel::Info, "Game thread joined");

  // Shutdown subsystems
  Application::Shutdown();
  Logger::getInstance().Log(LogLevel::Debug,
                            "Application shutdown started in main");

  // Clean up the sound system on exit
  SoundManager::Instance().terminate();

  return 0;
}