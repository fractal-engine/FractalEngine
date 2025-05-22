/* engine/runtime/subsystem_list.h
   Registry wrapper: holds concrete subsystem singletons owned by Application.
   NOTE: we could later switch to templates or compile-time tuples if needed.
*/
#ifndef SUBSYSTEM_LIST_H
#define SUBSYSTEM_LIST_H

#include <memory>
#include <vector>
#include "editor/runtime/subsystem.h"

namespace runtime {
class SubsystemList {
public:
  template <class T, class... Args>
  T* emplace_back(Args&&... args) {
    auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
    T* raw = ptr.get();
    subs_.emplace_back(std::move(ptr));
    return raw;
  }

  bool init_all() {
    for (auto& s : subs_)
      if (!s->init())
        return false;
    return true;
  }

  void tick_all(double dt) {
    for (auto& s : subs_)
      s->tick(dt);
  }

  void shutdown_all() {
    for (auto it = subs_.rbegin(); it != subs_.rend(); ++it)
      (*it)->shutdown();
    subs_.clear();
  }

private:
  std::vector<std::unique_ptr<ISubsystem>> subs_;
};
}  // namespace runtime

#endif  // SUBSYSTEM_LIST_H
