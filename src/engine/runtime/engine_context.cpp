#include "engine_context.h"
#include "engine/core/logger.h"
#include "engine/renderer/renderer_graphics.h"
#include "engine/renderer/shaders/shader_manager.h"
#include "engine/runtime/subsystem_list.h"
#include "platform/input/input.h"
#include "platform/window_manager.h"

namespace EngineContext {

// ------------------------------------------------------------------
//  Process-wide singletons
// ------------------------------------------------------------------
static std::unique_ptr<::GraphicsRenderer> graphics_renderer_instance_;
static std::unique_ptr<::ShaderManager> shader_manager_instance_;
static std::unique_ptr<::Input> input_device_instance_;

// ------------------------------------------------------------------
//  Hot-reload registry (not used yet)
// ------------------------------------------------------------------
static SubsystemList dynamic_registry;

// ------------------------------------------------------------------
//  Init / Tick / Shutdown
// ------------------------------------------------------------------
bool Init() {
  if (!WindowManager::Initialize())
    return false;

  graphics_renderer_instance_ = std::make_unique<::GraphicsRenderer>();
  if (!graphics_renderer_instance_->InitBgfx())
    return false;

  shader_manager_instance_ = std::make_unique<::ShaderManager>();
  shader_manager_instance_->Init();

  input_device_instance_ = std::make_unique<::Input>();

  // TODO: for future subsystems only (hot-reload, live editing, etc)
  return dynamic_registry.InitAll();
}

bool Tick() {
  using clock = std::chrono::steady_clock;
  static auto previous = clock::now();
  auto now = clock::now();
  double dt = std::chrono::duration<double>(now - previous).count();
  previous = now;

  dynamic_registry.TickAll(dt);
  return !::WindowManager::WindowShouldClose();
}

void Shutdown() {
  dynamic_registry.ShutdownAll();

  input_device_instance_.reset();
  shader_manager_instance_.reset();
  graphics_renderer_instance_.reset();
}

// ------------------------------------------------------------------
//  Accessors
// ------------------------------------------------------------------
::WindowManager& Window() {
  return WindowManager::getInstance();
}

::RendererBase& Renderer() {
  return *graphics_renderer_instance_;
}
::Input& Input() {
  return *input_device_instance_;
}
::ShaderManager& Shader() {
  return *shader_manager_instance_;
}

}  // namespace EngineContext