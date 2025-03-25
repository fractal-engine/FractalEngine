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

  // Start the game manager in a separate thread
  std::thread game_thread([&] { SubsystemManager::GetGameManager()->Run(); });

  // Log the start of the editor display
  Logger::getInstance().Log(LogLevel::DEBUG, "Initializing editor display");

  // Initialize the sound system.
  if (!SoundManager::Instance().init()) {
    Logger::getInstance().Log(LogLevel::ERROR,
                              "Failed to initialize SoundManager");
    return -1;
  }

  // Optionally adjust ambient volume (e.g., to 10%).
  SoundManager::Instance().setAmbientVolume(0.1f);

  // Start the ambient background sound.
  if (!SoundManager::Instance().startAmbient()) {
    Logger::getInstance().Log(LogLevel::ERROR, "Failed to start ambient sound");
    // Decide whether to exit or continue without ambient sound.
  }

  // Run the editor
  SubsystemManager::GetEditor()->Run();

  // Log the termination of the editor thread
  // Logger::getInstance().Log(LogLevel::INFO, "Editor thread terminated");

  // Join the game thread
  game_thread.join();

  // Log the joining of the game thread
  Logger::getInstance().Log(LogLevel::INFO, "Game thread joined");

  // Clean up the sound system on exit.
  SoundManager::Instance().terminate();

  return 0;
}
