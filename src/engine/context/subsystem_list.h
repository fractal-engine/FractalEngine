#ifndef SUBSYSTEM_LIST_H
#define SUBSYSTEM_LIST_H

#include <memory>
#include <vector>
#include "subsystem.h"

/* engine/context/subsystem_list.h
   Registry wrapper: holds concrete subsystem singletons owned by Application.
   NOTE: we could later switch to templates or compile-time tuples if needed.
*/

namespace EngineContext {

class SubsystemList {
public:
  template <class T, class... Args>
  T* Add(Args&&... args) {
    auto subsystem_ptr = std::make_unique<T>(std::forward<Args>(args)...);
    T* raw_ptr = subsystem_ptr.get();
    subsystems_.emplace_back(std::move(subsystem_ptr));
    return raw_ptr;
  }
  
  bool InitAll() {
    for (auto& subsystem : subsystems_)
      if (!subsystem->Init())
        return false;
    return true;
  }
  
  // TODO: use Time.h instead!
  void TickAll(double delta_time) {
    for (auto& subsystem : subsystems_)
      subsystem->Tick(delta_time);
  }
  
  void ShutdownAll() {
    for (auto subsystem_it = subsystems_.rbegin(); 
         subsystem_it != subsystems_.rend(); 
         ++subsystem_it) {
      (*subsystem_it)->Shutdown();
    }
    subsystems_.clear();
  }

private:
  std::vector<std::unique_ptr<ISubsystem>> subsystems_;
};
}  // namespace EngineContext

#endif  // SUBSYSTEM_LIST_H