#ifndef MODEL_OPERATION_DATA_H
#define MODEL_OPERATION_DATA_H

namespace ProcModel {

// Base struct for per-operation parameter data.
// Exists solely to enable safe shared_ptr ownership of derived types.
// No virtual Apply() — handlers are registered separately in
// ModelOperationRegistry.
struct ModelOperationData {
  virtual ~ModelOperationData() = default;
};

}  // namespace ProcModel

#endif  // MODEL_OPERATION_DATA_H