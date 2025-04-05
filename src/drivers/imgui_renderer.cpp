#include "imgui_renderer.h"

#include <backends/imgui_impl_sdl2.h>
#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <imgui.h>
#include "base/logger.h"
#include "base/shader_utils.h"
#include "subsystem/window_manager.h"

// static member initializations
bgfx::ProgramHandle ImGuiRenderer::imguiProgram = BGFX_INVALID_HANDLE;
bgfx::VertexLayout ImGuiRenderer::imguiVertexLayout;

void ImGuiRenderer::Init() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  SDL_Window* window = WindowManager::GetWindow();
  ImGui_ImplSDL2_InitForOther(window);

  imguiVertexLayout.begin()
      .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
      .end();

  // Get BGFX backend type to determine folder
  bgfx::ShaderHandle vs = loadShader("vs_imgui.bin");
  bgfx::ShaderHandle fs = loadShader("fs_imgui.bin");

  imguiProgram = bgfx::createProgram(vs, fs, true);

  Logger::getInstance().Log(LogLevel::Info, "ImGuiRenderer initialized.");
}

void ImGuiRenderer::BeginFrame() {
  SDL_Window* window = WindowManager::GetWindow();
  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize = ImVec2((float)WindowManager::GetWidth(),
                          (float)WindowManager::GetHeight());

  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
}

void ImGuiRenderer::EndFrame() {
  ImGui::Render();
  ImDrawData* drawData = ImGui::GetDrawData();
  if (!drawData || drawData->TotalVtxCount == 0) {

    bgfx::frame();
    return;
  }

  const float width = drawData->DisplaySize.x;
  const float height = drawData->DisplaySize.y;

  // Setup projection matrix (orthographic)
  float ortho[16];
  bx::mtxOrtho(ortho, 0.0f, width, height, 0.0f, 0.0f, 1000.0f, 0.0f, false);

  bgfx::setViewTransform(viewId_, NULL, ortho);
  bgfx::setViewRect(viewId_, 0, 0, uint16_t(width), uint16_t(height));
  bgfx::setViewClear(viewId_, BGFX_CLEAR_NONE);
  bgfx::touch(viewId_);

  for (int n = 0; n < drawData->CmdListsCount; ++n) {
    const ImDrawList* cmdList = drawData->CmdLists[n];

    const auto vtxSize = cmdList->VtxBuffer.Size * sizeof(ImDrawVert);
    const auto idxSize = cmdList->IdxBuffer.Size * sizeof(ImDrawIdx);

    const bgfx::Memory* vtxMem = bgfx::alloc(vtxSize);
    memcpy(vtxMem->data, cmdList->VtxBuffer.Data, vtxSize);
    bgfx::VertexBufferHandle vbo =
        bgfx::createVertexBuffer(vtxMem, imguiVertexLayout);

    const bgfx::Memory* idxMem = bgfx::alloc(idxSize);
    memcpy(idxMem->data, cmdList->IdxBuffer.Data, idxSize);
    bgfx::IndexBufferHandle ibo = bgfx::createIndexBuffer(idxMem);

    int32_t idxOffset = 0;

    for (int cmd_i = 0; cmd_i < cmdList->CmdBuffer.Size; cmd_i++) {
      const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd_i];

      const uint16_t scissorX = (uint16_t)pcmd->ClipRect.x;
      const uint16_t scissorY = (uint16_t)pcmd->ClipRect.y;
      const uint16_t scissorW = (uint16_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
      const uint16_t scissorH = (uint16_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);

      bgfx::setScissor(scissorX, scissorY, scissorW, scissorH);
      bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                     BGFX_STATE_MSAA | BGFX_STATE_BLEND_ALPHA);
      bgfx::setVertexBuffer(0, vbo);
      bgfx::setIndexBuffer(ibo, idxOffset, pcmd->ElemCount);
      bgfx::submit(viewId_, imguiProgram);

      idxOffset += pcmd->ElemCount;
    }

    bgfx::destroy(vbo);
    bgfx::destroy(ibo);
  }

  bgfx::frame();
}

void ImGuiRenderer::Shutdown() {
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
}
