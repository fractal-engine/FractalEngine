// ---------------------------------------------------------------------------
// ImGuiRenderer - Custom integration layer between Dear ImGui and BGFX
// References:
// - https://github.com/pr0g/sdl-bgfx-imgui-starter (imgui_impl_bgfx.cpp)
// - Original Gist by Richard Gale:
// https://gist.github.com/RichardGale/6e2b74bc42b3005e08397236e4be0fd0
// - BGFX + SDL2 + Dear ImGui integration notes from ocornut/imgui
//
// Purpose: Provides customized rendering logic to bridge ImGui with the BGFX
// backend, allowing reuse of shared engine rendering subsystems while
// respecting BGFX architecture.
// ---------------------------------------------------------------------------

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
bgfx::TextureHandle ImGuiRenderer::fontTexture = BGFX_INVALID_HANDLE;
bgfx::UniformHandle ImGuiRenderer::s_texUniform = BGFX_INVALID_HANDLE;

void ImGuiRenderer::Init() {
  IMGUI_CHECKVERSION();
  ImGui::StyleColorsDark();

  SDL_Window* window = WindowManager::GetWindow();
  ImGui_ImplSDL2_InitForOther(WindowManager::GetWindow());

  imguiVertexLayout.begin()
      .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
      .end();

  // Get BGFX backend type to determine folder
  bgfx::ShaderHandle vs = loadShader("vs_imgui.bin");
  bgfx::ShaderHandle fs = loadShader("fs_imgui.bin");

  // Create the uniform once
  if (!bgfx::isValid(s_texUniform)) {
    s_texUniform =
        bgfx::createUniform("g_AttribLocationTex", bgfx::UniformType::Sampler);
  }

  imguiProgram = bgfx::createProgram(vs, fs, true);

  unsigned char* pixels;
  int width, height;
  ImGuiIO& io = ImGui::GetIO();
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

  const bgfx::Memory* mem = bgfx::copy(pixels, width * height * 4);

  // format should be BGRA8
  fontTexture = bgfx::createTexture2D(
      uint16_t(width), uint16_t(height), false, 1, bgfx::TextureFormat::BGRA8,
      BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT, mem);

  // validate the texture
  if (!bgfx::isValid(fontTexture)) {
    Logger::getInstance().Log(LogLevel::Error,
                              "Font texture failed to upload.");
  }

  // Let ImGui know the font texture was submitted manually
  io.Fonts->TexID = static_cast<ImTextureID>(uintptr_t(fontTexture.idx));

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
    bgfx::touch(viewId_);
    bgfx::frame();
    return;
  }

  const float width = drawData->DisplaySize.x;
  const float height = drawData->DisplaySize.y;

  float ortho[16];
  bx::mtxOrtho(ortho, 0.0f, width, height, 0.0f, 0.0f, 1000.0f, 0.0f, false);

  bgfx::setViewRect(viewId_, 0, 0, uint16_t(width), uint16_t(height));
  bgfx::setViewTransform(viewId_, nullptr, ortho);

  // Set the view to clear once
  static bool once = false;
  if (!once) {
    bgfx::setViewClear(viewId_, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x2d2d2dff,
                       1.0f, 0);
    once = true;
  }

  bgfx::touch(viewId_);

  const ImGuiIO& io = ImGui::GetIO();
  int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
  int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
  if (fb_width == 0 || fb_height == 0)
    return;

  drawData->ScaleClipRects(io.DisplayFramebufferScale);

  // Set up orthographic projection
  const bgfx::Caps* caps = bgfx::getCaps();
  bx::mtxOrtho(ortho, 0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, 0.0f,
               1000.0f, 0.0f, caps->homogeneousDepth);

  bgfx::setViewTransform(viewId_, nullptr, ortho);
  bgfx::setViewRect(viewId_, 0, 0, uint16_t(fb_width), uint16_t(fb_height));
  bgfx::touch(viewId_);

  const uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                         BGFX_STATE_MSAA |
                         BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA,
                                               BGFX_STATE_BLEND_INV_SRC_ALPHA);

  for (int n = 0; n < drawData->CmdListsCount; n++) {
    const ImDrawList* cmd_list = drawData->CmdLists[n];

    uint32_t numVertices = (uint32_t)cmd_list->VtxBuffer.Size;
    uint32_t numIndices = (uint32_t)cmd_list->IdxBuffer.Size;

    if (numVertices == 0 || numIndices == 0)
      continue;

    if ((numVertices >
         bgfx::getAvailTransientVertexBuffer(numVertices, imguiVertexLayout)) ||
        (numIndices > bgfx::getAvailTransientIndexBuffer(numIndices))) {
      Logger::getInstance().Log(LogLevel::Warning,
                                "Not enough space in transient buffers.");
      continue;
    }

    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer tib;

    bgfx::allocTransientVertexBuffer(&tvb, numVertices, imguiVertexLayout);
    bgfx::allocTransientIndexBuffer(&tib, numIndices);

    memcpy(tvb.data, cmd_list->VtxBuffer.Data,
           numVertices * sizeof(ImDrawVert));
    memcpy(tib.data, cmd_list->IdxBuffer.Data, numIndices * sizeof(ImDrawIdx));

    int idx_offset = 0;
    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

      if (pcmd->UserCallback) {
        pcmd->UserCallback(cmd_list, pcmd);
      } else {
        const uint16_t xx = (uint16_t)bx::max(pcmd->ClipRect.x, 0.0f);
        const uint16_t yy = (uint16_t)bx::max(pcmd->ClipRect.y, 0.0f);
        const uint16_t ww = (uint16_t)bx::min(pcmd->ClipRect.z, 65535.0f) - xx;
        const uint16_t hh = (uint16_t)bx::min(pcmd->ClipRect.w, 65535.0f) - yy;

        bgfx::setScissor(xx, yy, ww, hh);
        bgfx::setState(state);
        bgfx::setVertexBuffer(0, &tvb);
        bgfx::setIndexBuffer(&tib, idx_offset, pcmd->ElemCount);

        bgfx::TextureHandle tex = {(uint16_t)(intptr_t)pcmd->TextureId};
        bgfx::setTexture(0, s_texUniform, tex);
        bgfx::submit(viewId_, imguiProgram);
      }
      idx_offset += pcmd->ElemCount;
    }
  }
  bgfx::frame();
}

void ImGuiRenderer::Shutdown() {
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  if (bgfx::isValid(s_texUniform)) {
    bgfx::destroy(s_texUniform);
    s_texUniform = BGFX_INVALID_HANDLE;
  }

  if (bgfx::isValid(fontTexture)) {
    bgfx::destroy(fontTexture);
    fontTexture = BGFX_INVALID_HANDLE;
  }

  if (bgfx::isValid(imguiProgram)) {
    bgfx::destroy(imguiProgram);
    imguiProgram = BGFX_INVALID_HANDLE;
  }
}