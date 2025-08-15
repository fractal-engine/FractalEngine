/**************************************************************************
 * EngineContext
 * -------------
 * Central access point for low-level engine systems and services.
 *
 * Owns / lifetime-managed:
 *  - GraphicsRenderer (graphics_renderer_instance_)
 *  - ShaderManager (shader_manager_instance_)
 *  - Input (input_device_instance_)
 *  - ResourceManager (resource_manager_)
 *  - SubsystemList hot-reload registry (dynamic_registry)
 *
 * Initializes / coordinates (not owned):
 *  - WindowManager (Initialize, WindowShouldClose)
 *  - ECS (entt::locator<ECS>::emplace)
 *  - SoundManager (Instance().init / Instance().terminate)
 *
 * Lifecycle:
 *  - Init(): Creates subsystems, initializes WindowManager, BGFX, ECS, Audio
 *  - Running(): Updates resources, ticks dynamic registry, polls window state
 *  - Destroy(): Shuts down subsystems in reverse dependency order
 *
 * Accessors:
 *  - Window(): Access to window and display operations
 *  - Renderer(): Access to rendering pipeline
 *  - InputDevice(): Access to input handling
 *  - Shader(): Access to shader management
 *  - resourceManager(): Access to resource
 **************************************************************************/

#include "engine_context.h"
#include "engine/audio/sound_manager.h"
#include "engine/context/subsystem_list.h"
#include "engine/core/logger.h"
#include "engine/ecs/world.h"
#include "engine/renderer/graphics_renderer.h"
#include "engine/renderer/shaders/shader_manager.h"
#include "platform/input/input.h"
#include "platform/window_manager.h"

namespace EngineContext {

// ------------------------------------------------------------------
//  Process-wide singletons
// ------------------------------------------------------------------
static std::unique_ptr<GraphicsRenderer> graphics_renderer_instance_;
static std::unique_ptr<ShaderManager> shader_manager_instance_;
static std::unique_ptr<Input> input_device_instance_;
static std::unique_ptr<ResourceManager> resource_manager_;

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

  graphics_renderer_instance_ = std::make_unique<GraphicsRenderer>();
  if (!graphics_renderer_instance_->InitBgfx())
    return false;

  shader_manager_instance_ = std::make_unique<ShaderManager>();
  shader_manager_instance_->Init();

  input_device_instance_ = std::make_unique<Input>();

  resource_manager_ = std::make_unique<ResourceManager>();

  // Initialize ECS singleton
  entt::locator<ECS>::emplace();

  // Initialize audio
  // TODO: rename to AudioContext
  if (!SoundManager::Instance().init()) {
    Logger::getInstance().Log(LogLevel::Error,
                              "Failed to initialize SoundManager");
    return false;
  }

  // TODO: for future subsystems only (hot-reload, live editing, etc)
  return dynamic_registry.InitAll();
}

bool Running() {
  using clock = std::chrono::steady_clock;
  static auto previous = clock::now();
  auto now = clock::now();
  double dt = std::chrono::duration<double>(now - previous).count();
  previous = now;

  if (resource_manager_) {
    resource_manager_->UpdateContext();
  }

  dynamic_registry.TickAll(dt);
  return !WindowManager::WindowShouldClose();
}

void Destroy() {
  dynamic_registry.ShutdownAll();

  resource_manager_.reset();
  input_device_instance_.reset();
  shader_manager_instance_.reset();
  graphics_renderer_instance_.reset();

  SoundManager::Instance().terminate();
}

// ------------------------------------------------------------------
//  Accessors
// ------------------------------------------------------------------
WindowManager& Window() {
  return WindowManager::getInstance();
}

RendererBase& Renderer() {
  return *graphics_renderer_instance_;
}

Input& InputDevice() {
  return *input_device_instance_;
}

ShaderManager& Shader() {
  return *shader_manager_instance_;
}

ResourceManager& resourceManager() {
  return *resource_manager_;
}

}  // namespace EngineContext