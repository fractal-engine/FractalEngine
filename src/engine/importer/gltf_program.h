#pragma once
#include <bgfx/bgfx.h>

// Declaration only (defined in game_test.cpp)
extern bgfx::ProgramHandle g_gltfProgram;

namespace GltfImport {
void SetupGltfLayouts();
void SetupGltfProgram();
}  // namespace GltfImport