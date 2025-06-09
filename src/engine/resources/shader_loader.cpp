#include <bgfx/bgfx.h>
#include <bx/debug.h>
#include <bx/file.h>
#include <bx/readerwriter.h>

#include "engine/core/logger.h"

#include "shader_utils.h"

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
  Logger::getInstance().Log(LogLevel::Debug, std::string("Shader file size: ") +
                                                 std::to_string(fileSize));
  const bgfx::Memory* mem =
      bgfx::alloc(fileSize + 1);  // Allocate memory for shader file
  bx::Error err;

  bx::read(&fileReader, mem->data, fileSize, &err);  // Pass an error object

  if (err.isOk()) {
    // mem->data[fileSize] = '\0';  // Null-terminate
    bx::close(&fileReader);
    return bgfx::createShader(mem);
  } else {
    Logger::getInstance().Log(LogLevel::Error,
                              std::string("Failed to read shader file"));
    bx::close(&fileReader);
    return BGFX_INVALID_HANDLE;
  }
}

// New helper to load a program with optional fragment shader file
bgfx::ProgramHandle LoadProgram(const char* vsFile,
                                const char* fsFile = nullptr) {
  bgfx::ShaderHandle vs = loadShader(vsFile);
  if (!bgfx::isValid(vs)) {
    Logger::getInstance().Log(
        LogLevel::Error,
        "Failed to load vertex shader: " + std::string(vsFile));
    return BGFX_INVALID_HANDLE;
  }

  bgfx::ShaderHandle fs = BGFX_INVALID_HANDLE;

  if (fsFile && fsFile[0] != '\0') {
    fs = loadShader(fsFile);
    if (!bgfx::isValid(fs)) {
      Logger::getInstance().Log(
          LogLevel::Error,
          "Failed to load fragment shader: " + std::string(fsFile));
      bgfx::destroy(vs);
      return BGFX_INVALID_HANDLE;
    }
  }

  // true means shaders will be destroyed when program is destroyed
  return bgfx::createProgram(vs, fs, true);
}
