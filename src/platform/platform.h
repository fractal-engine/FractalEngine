#ifndef PLATFORM_H
#define PLATFORM_H

/*  platform/platform.h
    Thin interface used by engine to communicate to OS layer.
    For now it just forwards access to SDL
*/
class Platform {
public:
  virtual ~Platform() = default;

  // Process OS events; returns false if user requests quit
  virtual bool pumpEvents() = 0;

  // Resize/DPI queries go here
};

#endif  // PLATFORM_H
