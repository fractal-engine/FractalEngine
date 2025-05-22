#ifndef APPLICATION_H
#define APPLICATION_H

// Engine Manager is used to manage all the subsystems/engines inside the game
// engine. It's responsible for managing the initialization and destruction of
// the subsystems and the interaction between them.

#include <memory>
#include <mutex>
#include <thread>

#include "engine/core/singleton.hpp"

#include "editor/editor_base.h"   // abstract interface
#include "editor/editor_layer.h"  // concrete implementation
#include "editor/runtime/subsystem.h"
#include "editor/runtime/subsystem_list.h"
#include "engine/renderer/renderer_base.h"      // abstract interface
#include "engine/renderer/renderer_graphics.h"  // concrete implementation
#include "engine/renderer/shaders/shader_manager.h"
#include "game/game_manager.h"
#include "platform/input/input.h"
#include "platform/window_manager.h"  // window manager

class Application : public Singleton<Application> {
private:
  std::unique_ptr<EditorBase> editor_;
  std::unique_ptr<RendererBase> renderer_;
  std::unique_ptr<GameManager> game_manager_;
  std::unique_ptr<Input> input_;
  std::unique_ptr<WindowManager> window_manager_;
  std::unique_ptr<ShaderManager> shader_manager_;

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

#endif  // APPLICATION_H
