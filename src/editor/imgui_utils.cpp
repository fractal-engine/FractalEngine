#include "imgui_utils.h"

#include "editor/runtime/application.h"
#include "engine/core/logger.h"
#include "engine/core/view_ids.h"
#include "platform/window_manager.h"

#include <backends/imgui_impl_sdl2.h>
#include <imgui.h>
#include "editor/vendor/imgui/imgui_impl_bgfx.h"

extern "C" void ImGui_Implbgfx_SetCustomShader(bgfx::ProgramHandle handle);

void ApplyImGuiShaders() {
  auto handle = Application::GetShaderManager()->LoadProgram(
      "imgui", "vs_imgui.bin", "fs_imgui.bin");

  if (!bgfx::isValid(handle)) {
    Logger::getInstance().Log(
        LogLevel::Error, "Failed to load ImGui shaders via ShaderManager.");
  } else {
    ImGui_Implbgfx_SetCustomShader(handle);
    ImGui_Implbgfx_Init(ViewID::UI);
  }
}