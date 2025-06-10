#ifndef CORE_LOOP_H
#define CORE_LOOP_H

#include <memory>
#include "engine/renderer/renderer_base.h"
#include "engine/renderer/shaders/shader_manager.h"
#include "platform/input/input.h"
#include "platform/window_manager.h"

class RendererBase;
class Input;
class ShaderManager;

namespace runtime {

// ---------------------- life-cycle ----------------
bool Init();
bool Tick();  // returns false when SDL quit requested
void Shutdown();

// --------------- global singletons ----------------
::WindowManager& Window();
::RendererBase& Renderer();
::Input& Input();
::ShaderManager& Shader();

}  // namespace runtime
#endif  // CORE_LOOP_H
