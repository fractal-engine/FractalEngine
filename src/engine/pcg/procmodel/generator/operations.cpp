#include "operations.h"

#include "engine/pcg/pipeline/operation_context.h"
#include "engine/pcg/procmodel/generator/operations/duplicate_instances.h"
#include "engine/pcg/procmodel/generator/operations/vertex_deform.h"

namespace ProcModel {

void RegisterModelOperations(PCG::OperationRegistry& registry) {
  RegisterDuplicateInstancesOperation(registry);
  RegisterVertexDeformOperation(registry);
}

}  // namespace ProcModel