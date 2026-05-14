#ifndef DUPLICATE_INSTANCES_H
#define DUPLICATE_INSTANCES_H

#include <cstdint>
#include <string>

#include "engine/pcg/pipeline/operation_data.h"

namespace PCG {
class OperationRegistry;
}

namespace ProcModel {

struct DuplicateInstancesData : public PCG::OperationData {
  std::string target_group_id;
  std::uint32_t min_count = 1;
  std::uint32_t max_count = 1;
};

void RegisterDuplicateInstancesOperation(PCG::OperationRegistry& registry);

}  // namespace ProcModel

#endif  // DUPLICATE_INSTANCES_H