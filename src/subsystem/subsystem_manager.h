#ifndef SUBSYSTEM_MANAGER_H
#define SUBSYSTEM_MANAGER_H

#include <memory>
#include <mutex>
#include <thread>

#include "core/singleton.hpp"


#include "renderer/shaders/shader_manager.h"

#include "subsystem/engine_implements.h.in"
#include "subsystem/game_manager.h"
#include "subsystem/input/input.h"
#include "subsystem/window_manager.h"

// Engine Manager is used to manage all the subsystems/engines inside the game
// engine. It's responsible for managing the initialization and destruction of
// the subsystems and the interaction between them.
class SubsystemManager : public Singleton<SubsystemManager> {
private:
  std::unique_ptr<EditorBase> editor_;
  std::unique_ptr<RendererBase> renderer_;
  std::unique_ptr<GameManager> game_manager_;
  std::unique_ptr<Input> input_;
  std::unique_ptr<WindowManager> window_manager_;
  std::unique_ptr<ShaderManager> shader_manager_;

  // initialize threads
  void initialize();

public:
  static void Initialize();
  static void Shutdown();

  static const std::unique_ptr<EditorBase>& GetEditor();
  static const std::unique_ptr<RendererBase>& GetRenderer();
  static const std::unique_ptr<GameManager>& GetGameManager();
  static const std::unique_ptr<Input>& GetInput();
  static const std::unique_ptr<WindowManager>& GetWindowManager();
  static const std::unique_ptr<ShaderManager>& GetShaderManager();
};

#endif  // SUBSYSTEM_MANAGER_H
