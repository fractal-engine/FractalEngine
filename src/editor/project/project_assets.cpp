#include "project_assets.h"

#include "editor/runtime/application.h"
#include "engine/core/logger.h"
#include "engine/importer/gltf_program.h"

void GltfImport::SetupGltfProgram() {
  if (bgfx::isValid(g_gltfProgram))
    return;

  ShaderManager& sm = *Application::GetShaderManager();
  g_gltfProgram = sm.LoadProgram("gltf_default", "vs_gltf.bin", "fs_gltf.bin");

  if (!bgfx::isValid(g_gltfProgram))
    Logger::getInstance().Log(LogLevel::Error,
                              "Failed to create glTF shader program");
}