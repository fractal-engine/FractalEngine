#include "game/game_test.h"
#include <thirdparty/bgfx.cmake/bgfx/include/bgfx/bgfx.h>  // Include BGFX library for rendering
#include "base/logger.h"        // Include Logger for logging
#include "base/shader_utils.h"  // Include ShaderUtils for shader loading

// Constructor: Initializes _helloWorldProgram to an invalid BGFX handle
GameTest::GameTest() : _helloWorldProgram(BGFX_INVALID_HANDLE) {}

// Destructor: Destroys the shader program if it is valid
GameTest::~GameTest() {
  if (bgfx::isValid(_helloWorldProgram))  // Check if the program is valid
    bgfx::destroy(_helloWorldProgram);    // Release the program resources
}

// Initialization function: Loads shaders and creates a BGFX program
void GameTest::Init() {
  // Load vertex shader using custom loadShader function
  bgfx::ShaderHandle vs = loadShader("vs_terrain.bin");
  if (!bgfx::isValid(vs)) {  // Check if the vertex shader handle is valid
    Logger::getInstance().Log(LogLevel::ERROR,
                              "Failed to load vertex shader: vs_terrain.bin");
    return;  // Exit function if shader loading fails
  }

  // Load fragment shader using custom loadShader function
  bgfx::ShaderHandle fs = loadShader("fs_terrain.bin");
  if (!bgfx::isValid(fs)) {  // Check if the fragment shader handle is valid
    Logger::getInstance().Log(LogLevel::ERROR,
                              "Failed to load fragment shader: fs_terrain.bin");
    return;  // Exit function if shader loading fails
  }

  // Create a BGFX program using the loaded vertex and fragment shaders
  _helloWorldProgram = bgfx::createProgram(vs, fs, true);
  if (!bgfx::isValid(
          _helloWorldProgram)) {  // Check if the program handle is valid
    Logger::getInstance().Log(LogLevel::ERROR,
                              "Failed to create shader program.");
    return;  // Exit function if program creation fails
  }

  // Log success message after shaders are loaded and program is created
  Logger::getInstance().Log(LogLevel::INFO,
                            "Hello World: Shader program loaded successfully.");
}

// Update function: Handles rendering every frame
void GameTest::Update() {
  // Validate the shader program handle before rendering
  if (!bgfx::isValid(_helloWorldProgram)) {
    Logger::getInstance().Log(LogLevel::ERROR, "Shader program is invalid.");
    return;  // Exit function if shader program is invalid
  }

  // Set up the view clear behavior for view ID 0
  bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000FF, 1.0f,
                     0);

  // Mark view ID 0 as touched (ensures it gets processed in the rendering
  // pipeline)
  bgfx::touch(0);

  // Log rendering progress
  Logger::getInstance().Log(LogLevel::INFO,
                            "Rendering frame with Hello World shader.");
}