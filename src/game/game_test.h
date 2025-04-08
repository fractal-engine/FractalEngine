#ifndef GAME_TEST_H
#define GAME_TEST_H

#include <bgfx/bgfx.h>
#include "base/game_base.h"

// A minimal game class that just displays "Hello World".
class GameTest : public Game {
public:
  GameTest();
  virtual ~GameTest();

  // Initialize the shader program (and any minimal resources)
  // that will be used to display "Hello World".
  void Init() override;

  // Update is called every frame�in this minimal example, it just
  // submits a BGFX touch call and logs a message.
  void Update() override;

  void Shutdown() override;

private:
  // BGFX resources
  bgfx::ProgramHandle _helloWorldProgram;
  bgfx::VertexBufferHandle vertexBuffer = BGFX_INVALID_HANDLE;
  bgfx::IndexBufferHandle indexBuffer = BGFX_INVALID_HANDLE;
  float world_matrix[16];  // 4x4 transformation matrix
};

#endif  // GAME_TEST_H