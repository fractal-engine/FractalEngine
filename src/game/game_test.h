#ifndef GAME_TEST_H
#define GAME_TEST_H

#include <bgfx/bgfx.h>
#include "game/game_base.h"

#include <vector>

// vertex definition for terrain
struct PosTexCoord0Vertex {
  float x, y, z;
  float u, v;

  static bgfx::VertexLayout layout;  // Declaration
  static void init();
};

// A minimal game class that just displays "Hello World".
class GameTest : public GameBase {
public:
  GameTest();
  virtual ~GameTest();

  // Initialize the shader program (and any minimal resources)
  // that will be used to display "Hello World".
  void Init() override;

  // Update is called every frame�in this minimal example, it just
  // submits a BGFX touch call and logs a message.
  void Update() override;

  void Render() override;

  void Shutdown() override;

  float cameraEye[3] = {120.0f, 60.0f, 32.0f};
  float cameraAt[3] = {32.0f, 0.0f, 32.0f};
  float cameraUp[3] = {1.0f, 0.0f, 0.0f};
  float cameraFOV = 80.0f;

private:
  // BGFX resources
  bgfx::ProgramHandle _terrainProgramHeight = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle _heightUniform;
  bgfx::TextureHandle _heightTexture;
  bgfx::UniformHandle _lightDirUniform = BGFX_INVALID_HANDLE;

  bgfx::VertexBufferHandle vertexBuffer = BGFX_INVALID_HANDLE;
  bgfx::IndexBufferHandle indexBuffer = BGFX_INVALID_HANDLE;

  std::vector<PosTexCoord0Vertex> terrainVertices;
  std::vector<uint16_t> terrainIndices;

  float world_matrix[16];  // 4x4 transformation matrix

  void* _terrainData = nullptr;
};

#endif  // GAME_TEST_H