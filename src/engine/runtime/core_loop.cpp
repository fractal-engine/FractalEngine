#include "core_loop.h"
#include "engine/core/logger.h"
#include "engine/renderer/renderer_graphics.h"
#include "engine/renderer/shaders/shader_manager.h"
#include "platform/input/input.h"
#include "platform/window_manager.h"

namespace runtime {

// ------------------------------------------------------------------
//  Static singletons
// ------------------------------------------------------------------

static runtime::SubsystemList subs_;
static std::unique_ptr<::WindowManager> window_;
static std::unique_ptr<::GraphicsRenderer> renderer_;
static std::unique_ptr<::ShaderManager> shaders_;
static std::unique_ptr<::Input> input_;

// ------------------------------------------------------------------
//  Init / Tick / Shutdown
// ------------------------------------------------------------------

bool Init(const Config& config) {
  window_ = std::make_unique<::WindowManager>();
  if (!window_->Initialize(config.window_title, config.width, config.height))
    return false;

  renderer_ = std::make_unique<::GraphicsRenderer>();
  if (!renderer_->InitBgfx())
    return false;

  shaders_ = std::make_unique<::ShaderManager>();
  shaders_->Init();
  input_ = std::make_unique<::Input>();

  subs_.RegisterSubsystem<::WindowManager>(window_.get());
  subs_.RegisterSubsystem<::Input>(input_.get());
  subs_.RegisterSubsystem<::GraphicsRenderer>(renderer_.get());
  return subs_.InitAll();
}

bool Tick() {
  using clock = std::chrono::steady_clock;
  static auto previous = clock::now();
  auto now = clock::now();
  double dt = std::chrono::duration<double>(now - previous).count();
  previous = now;

  subs_.TickAll(dt);
  return !::WindowManager::WindowShouldClose();
}

void Shutdown() {
  subs_.ShutdownAll();
  shaders_.reset();
  renderer_.reset();
  input_.reset();
  window_.reset();
}

// ------------------------------------------------------------------
//  Getters
// ------------------------------------------------------------------

::WindowManager& Window() {
  return *window_;
}
::RendererBase& Renderer() {
  return *renderer_;
}  // implicit up-cast
::Input& InputDevice() {
  return *input_;
}
::ShaderManager& ShaderManager() {
  return *shaders_;
}

}  // namespace runtime