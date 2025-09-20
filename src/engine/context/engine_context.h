#ifndef ENGINE_CONTEXT_H
#define ENGINE_CONTEXT_H

#include <memory>
#include "engine/renderer/renderer_base.h"
#include "engine/renderer/shaders/shader_manager.h"
#include "platform/input/input.h"
#include "platform/window_manager.h"
#include "engine/memory/resource_manager.h"
#include "engine/time/time.h"

class RendererBase;
class Input;
class ShaderManager;

namespace EngineContext {

// ---------------------- life-cycle ----------------
bool Init(); // TODO: rename to Create later? 
bool Running();  // returns false when SDL quit requested
void Destroy();

// --------------- global singletons ----------------
WindowManager& Window();
RendererBase& Renderer();
Input& InputDevice();
ShaderManager& Shader();
ResourceManager& resourceManager();

}  // namespace EngineContext
#endif  // ENGINE_CONTEXT_H
