#ifndef GAME_TEST_H
#define GAME_TEST_H
#include "base/game_base.h"

#include <string>
#include <vector>

class GameTest : public Game {
  int interval = 0;
  int pos_x = 0;
  int pos_y = 0;
  std::vector<std::string> asciiArt = {
      R"(==============================)",  //
      R"(       _                   _  )",  //
      R"(   ___| |__   __ _ _ __ __| | )",  //
      R"(  / __| '_ \ / _` | '__/ _` | )",  //
      R"(  \__ \ | | | (_| | | | (_| | )",  //
      R"(  |___/_| |_|\__,_|_|  \__,_| )",  //
      R"(                              )",  //
      R"(          It Works!           )",  //
      R"(                              )",  //
      R"(==============================)",  //
  };

public:
  void Update() override;
};
#endif