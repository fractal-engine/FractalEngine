#include <thirdparty/bgfx.cmake/bgfx/include/bgfx/bgfx.h>
#include <thirdparty/bgfx.cmake/bx/include/bx/debug.h>
#include <thirdparty/bgfx.cmake/bx/include/bx/file.h>
#include <thirdparty/bgfx.cmake/bx/include/bx/readerwriter.h>
#include "base/logger.h"
#include "base/shader_utils.h"

bgfx::ShaderHandle loadShader(const char* filePath) {
  // Add debug output for shader paths
  Logger::getInstance().Log(
      LogLevel::Debug, "Attempting to load shader: " + std::string(filePath));

  std::string fullPath;
  const bgfx::RendererType::Enum rendererType = bgfx::getRendererType();
  const std::string folder = GetShaderFolder(rendererType);
  
  // Construct path based on detected backend
  fullPath = "assets/shaders/" + folder + "/" + filePath;

  Logger::getInstance().Log(LogLevel::Debug, 
      "Resolved shader path: " + fullPath);

  bx::FileReader fileReader;
  if (!bx::open(&fileReader, fullPath.c_str())) {
    Logger::getInstance().Log(
        LogLevel::Error, std::string("Shader file not found: ") + fullPath);
    return BGFX_INVALID_HANDLE;
  }

  uint32_t fileSize = (uint32_t)bx::getSize(&fileReader);
  const bgfx::Memory* mem =
      bgfx::alloc(fileSize + 1);  // Allocate memory for shader file
  bx::Error err;

  // Correct call to bx::read
  bx::read(&fileReader, mem->data, fileSize, &err);  // Pass an error object

  if (err.isOk()) {
    mem->data[fileSize] = '\0';  // Null-terminate
    bx::close(&fileReader);
    return bgfx::createShader(mem);
  } else {
    bx::close(&fileReader);
    return BGFX_INVALID_HANDLE;
  }
}