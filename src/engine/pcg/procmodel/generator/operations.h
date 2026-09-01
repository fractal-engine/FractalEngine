#ifndef PROCMODEL_OPERATIONS_H
#define PROCMODEL_OPERATIONS_H

#include "engine/pcg/pipeline/operation_registry.h"

namespace ProcModel {

// Registers all built-in ProcModel operations (duplicate_instances,
// vertex_deform) into given registry. Called from Subsystem::Init().
void RegisterModelOperations(PCG::OperationRegistry& registry);

}  // namespace ProcModel

#endif  // PROCMODEL_OPERATIONS_H