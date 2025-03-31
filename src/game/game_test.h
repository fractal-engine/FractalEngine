#ifndef GAME_TEST_H
#define GAME_TEST_H

#include <thirdparty/bgfx.cmake/bgfx/include/bgfx/bgfx.h>
#include "base/game_base.h"

// A minimal game class that just displays "Hello World".
class GameTest : public Game {
public:
  GameTest();
  virtual ~GameTest();

  // Initialize the shader program (and any minimal resources)
  // that will be used to display "Hello World".
  void Init() override;

  // Update is called every frame—in this minimal example, it just
  // submits a BGFX touch call and logs a message.
  void Update() override;

private:
  bgfx::ProgramHandle _helloWorldProgram;
};

#endif  // GAME_TEST_H