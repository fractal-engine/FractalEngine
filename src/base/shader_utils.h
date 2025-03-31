#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include <thirdparty/bgfx.cmake/bgfx/include/bgfx/bgfx.h>

// Function to load a BGFX shader from a binary file
bgfx::ShaderHandle loadShader(const char* filePath);

#endif  // SHADER_UTILS_H