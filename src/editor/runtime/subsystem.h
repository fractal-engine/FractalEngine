/* engine/runtime/subsystem.h
   Common interface for all engine subsystems
   Most existing classes (Renderer, Input, etc.) already expose
*/

#ifndef SUBSYSTEM_H
#define SUBSYSTEM_H

#include <cstdint>

namespace runtime {
class ISubsystem {
public:
  virtual ~ISubsystem() = default;

  // Called once, after subsystem is constructed but before any Tick()
  virtual bool init() = 0;

  // Called every frame by Application::run()
  /// \param dt  frame delta-time in seconds
  virtual void tick(double dt) = 0;

  // Called once on shutdown, just before deletion.
  virtual void shutdown() = 0;
};
}  // namespace runtime

#endif  // SUBSYSTEM_H
