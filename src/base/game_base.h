#ifndef GAME_BASE_H
#define GAME_BASE_H

class GameBase {
public:
  virtual void Init() = 0;
  virtual void Update() = 0;
  virtual void Render() = 0;
  virtual void Shutdown() = 0;

  virtual ~GameBase() = default;
};

#endif  // GAME_BASE_H