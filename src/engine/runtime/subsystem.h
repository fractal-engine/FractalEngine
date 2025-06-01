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
  virtual bool Init() = 0;
  virtual void Tick(double dt) = 0;
  virtual void Shutdown() = 0;
};

}  // namespace runtime

#endif  // SUBSYSTEM_H
