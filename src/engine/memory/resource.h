#ifndef RESOURCE_H
#define RESOURCE_H

#include <atomic>
#include <cstdint>
#include <string>

#include "resource_pipe.h"

using ResourceID = uint32_t;

enum class ResourceState {
  EMPTY,    // Resource not initialized or loaded
  QUEUED,   // Resource queued for async loading
  LOADING,  // Resource being loaded (CPU or GPU)
  READY,    // Resource loaded and ready to use
  FAILED    // Resource failed to load
};

class Resource {
private:
  friend class ResourceManager;

  // Resource ID
  ResourceID resource_id_;

  // Resource name
  std::string resource_name_;

  // Current state
  std::atomic<ResourceState> resource_state_;

protected:
  // Create a new resource pipe for this resource
  ResourcePipe Pipe() { return ResourcePipe(resource_id_); }

  Resource()
      : resource_id_(0),
        resource_name_("none"),
        resource_state_(ResourceState::EMPTY) {}

public:
  virtual ~Resource() = default;

  // Clean up GPU resources
  virtual void Destroy() = 0;

  // Get resource ID
  ResourceID GetID() const { return resource_id_; }

  // Get resource name
  const std::string& GetName() const { return resource_name_; }

  // Get current state
  ResourceState GetState() const { return resource_state_; }
};

#endif  // RESOURCE_H