#ifndef ENGINE_CONTEXT_H
#define ENGINE_CONTEXT_H

#include <memory>

#include "engine/memory/resource_manager.h"
#include "engine/pcg/pcg_engine.h"
#include "engine/renderer/renderer_base.h"
#include "engine/renderer/shaders/shader_manager.h"
#include "engine/time/time.h"
#include "platform/input/input.h"
#include "platform/window_manager.h"

class RendererBase;
class Input;
class ShaderManager;
class PCGEngine;

namespace EngineContext {

// ---------------------- life-cycle ----------------
bool Init();     // TODO: rename to Create later?
bool Running();  // returns false when SDL quit requested
void Destroy();
void NextFrame();

// --------------- global singletons ----------------
WindowManager& Window();
RendererBase& Renderer();
Input& InputDevice();
ShaderManager& Shader();
ResourceManager& resourceManager();
PCGEngine& Generator();

}  // namespace EngineContext
#endif  // ENGINE_CONTEXT_H
