#ifndef PCG_OPERATION_DATA_H
#define PCG_OPERATION_DATA_H

namespace PCG {

// Base struct for per-operation parameter data.
// Enables safe shared_ptr ownership of derived types.
// No virtual Apply() — handlers are registered separately in OperationRegistry.
struct OperationData {
  virtual ~OperationData() = default;
};

}  // namespace PCG

#endif  // PCG_OPERATION_DATA_H