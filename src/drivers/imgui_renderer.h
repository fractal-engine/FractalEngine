#ifndef IMGUI_RENDERER_H
#define IMGUI_RENDERER_H

#include <bgfx/bgfx.h>

class ImGuiRenderer {
public:
  void Init();
  void BeginFrame();
  void EndFrame();
  void Shutdown();

private:

  // "constexpr uint8_t kImGuiViewId = 255;" has to be set globally instead
  const uint8_t viewId_ = 1; // TODO: Set a proper view ID, Imgui 

  // bgfx handles for shaders and vertex layout
  static bgfx::VertexLayout imguiVertexLayout;
  static bgfx::ProgramHandle imguiProgram;
  static bgfx::UniformHandle s_texUniform;
  static bgfx::TextureHandle fontTexture;
};

#endif  // IMGUI_RENDERER_H
