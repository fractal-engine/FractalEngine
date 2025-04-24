#ifndef IMGUI_BACKEND_H
#define IMGUI_BACKEND_H

#include <bgfx/bgfx.h>

class ImGuiBackend {
public:
  void Init();
  void BeginFrame();
  void EndFrame();
  void Shutdown();

private:
  // bgfx handles for shaders and vertex layout
  static bgfx::VertexLayout imguiVertexLayout;
  static bgfx::ProgramHandle imguiProgram;
  static bgfx::UniformHandle s_texUniform;
  static bgfx::TextureHandle fontTexture;
};

#endif  // IMGUI_BACKEND_H
