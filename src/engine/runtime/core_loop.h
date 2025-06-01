#ifndef CORE_LOOP_H
#define CORE_LOOP_H

#include <chrono>
#include <memory>
#include "engine/runtime/subsystem_list.h"

namespace runtime {
struct Config {
  const char* window_title = "Fractal Engine";
  int width = 1280;
  int height = 720;
  bool vsync = true;
};

/// Initialise all engine-level subsystems (window, bgfx, input, …)
bool Init(const Config&);

/// Advance one frame; returns false when the OS asks to quit
bool Tick();

/// Shut everything down in reverse order
void Shutdown();

/// Handy getters used by the editor façade
class WindowManager& window();
class RendererBase& renderer();
class Input& input();
class ShaderManager& shaderManager();

}  // namespace runtime
#endif  // CORE_LOOP_H