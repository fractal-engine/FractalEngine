#ifndef GAME_BASE_H
#define GAME_BASE_H

class Game {
public:
  virtual void Init() = 0;    // Add Init as a pure virtual method
  virtual void Update() = 0;  // Existing method in our old implementation
  virtual ~Game() = default;
};

#endif  // GAME_BASE_H