#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include "engine/core/singleton.hpp"

#include "editor/editor_base.h"
#include "editor/editor_layer.h"
#include "engine/renderer/renderer_base.h"
#include "engine/renderer/shaders/shader_manager.h"
#include "game/game_manager.h"
#include "platform/input/input.h"
#include "platform/window_manager.h"

class Application : public Singleton<Application> {
private:
  /* ── Editor-owned subsystem management ───────────────────── */
  std::unique_ptr<EditorBase> editor_;
  std::unique_ptr<GameManager> game_manager_;

  /* ── Runtime subsystem references  ──── */
  RendererBase* renderer_ = nullptr;
  ShaderManager* shader_manager_ = nullptr;
  Input* input_ = nullptr;
  WindowManager* window_manager_ = nullptr;

  void InitializeInternal();

public:
  static void Initialize();
  static void Shutdown();

  /* ---- accessors ------------ */
  static EditorBase* GetEditor();
  static RendererBase* GetRenderer();
  static GameManager* GetGameManager();
  static Input* GetInput();
  static WindowManager* GetWindowManager();
  static ShaderManager* GetShaderManager();
};

#endif  // APPLICATION_H
