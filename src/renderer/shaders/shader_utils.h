#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include <bgfx/bgfx.h>
#include <string>

// Function to get the shader folder based on the backend
inline std::string GetShaderFolder(bgfx::RendererType::Enum backend) {
  switch (backend) {
    case bgfx::RendererType::Direct3D11:
      return "dx11";
    case bgfx::RendererType::Direct3D12:
      return "dx12";
    case bgfx::RendererType::Metal:
      return "metal";
    case bgfx::RendererType::OpenGL:
      return "glsl";
    case bgfx::RendererType::Vulkan:
      return "spirv";
    default:
      return "spirv";  // fallback
  }
}

// Function to load a BGFX shader from a binary file
bgfx::ShaderHandle loadShader(const char* filePath);

#endif  // SHADER_UTILS_H