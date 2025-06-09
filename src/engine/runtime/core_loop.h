#ifndef CORE_LOOP_H
#define CORE_LOOP_H

#include <memory>
#include "engine/renderer/renderer_base.h"
#include "engine/renderer/shaders/shader_manager.h"
#include "platform/input/input.h"
#include "platform/window_manager.h"

class WindowManager;
class RendererBase;
class Input;
class ShaderManager;

namespace runtime {

// -------------------- config --------------------
struct Config {
  const char* window_title = "Fractal Engine";
  int width = 1280;
  int height = 720;
  bool vsync = true;
};

// ---------------------- life-cycle ----------------
bool Init(const Config& config);
bool Tick();  // returns false when SDL quit requested
void Shutdown();

// --------------- global singletons ----------------
::WindowManager& Window();
::RendererBase& Renderer();
::Input& Input();
::ShaderManager& Shader();

}  // namespace runtime
#endif  // CORE_LOOP_H
