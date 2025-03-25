#ifndef GAME_BASE_H
#define GAME_BASE_H

class Game {
 public:
  virtual void Update() = 0;
  virtual ~Game() = default;
};

#endif  // GAME_BASE_H