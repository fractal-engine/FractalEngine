#ifndef PCG_OPERATION_CONTEXT_H
#define PCG_OPERATION_CONTEXT_H

#include <cassert>

#include "engine/pcg/pipeline/operation_data.h"

namespace PCG {

// Base context type for operation handlers.
// Concrete subsystems derive their own context types
//
// Pipeline never inspects the context itself,
// handlers downcast to the expected concrete type.
struct OperationContext {
  virtual ~OperationContext() = default;
};

// Checked-cast helper for handlers.
// Use the data form for downcasting OperationData; the context form for
// downcasting OperationContext.
//
// In debug builds (NDEBUG not defined), uses dynamic_cast and asserts on
// failure. In release builds, uses static_cast. The registry guarantees
// type safety so static_cast is correct in release.
template <typename T>
inline const T& OperationCast(const OperationData& base) {
#ifdef NDEBUG
  return static_cast<const T&>(base);
#else
  const T* ptr = dynamic_cast<const T*>(&base);
  // Failure here means the registry's parser produced a type that the
  // handler did not expect (registration mismatch).
  assert(ptr != nullptr && "OperationData type mismatch");
  return *ptr;
#endif
}

template <typename T>
inline T& OperationCast(OperationContext& base) {
#ifdef NDEBUG
  return static_cast<T&>(base);
#else
  T* ptr = dynamic_cast<T*>(&base);
  // Failure here means a pipeline was run against a context type from a
  // different subsystem (caller error)
  assert(ptr != nullptr && "OperationContext type mismatch");
  return *ptr;
#endif
}

}  // namespace PCG

#endif  // PCG_OPERATION_CONTEXT_H