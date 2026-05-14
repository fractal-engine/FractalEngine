#ifndef PROCMODEL_H
#define PROCMODEL_H

#include "engine/memory/resource.h"

#include "engine/pcg/pipeline/operation_registry.h"

#include "engine/pcg/procmodel/generator/operations.h"
#include "engine/pcg/procmodel/instantiator/model_instantiator.h"
#include "engine/pcg/procmodel/validation/validation_logger.h"

namespace ProcModel {

class Subsystem {
public:
  Subsystem();
  ~Subsystem() = default;

  void Init();
  void Shutdown();

  ResourceID LoadArchetype(const std::string& descriptor_path);

  ModelInstantiator::InstantiateResult RequestInstance(
      const std::string& descriptor_path, uint64_t seed,
      Entity parent = entt::null);

  ValidationLogger& ValidationLog();

  PCG::OperationRegistry& GetOperationRegistry() { return operation_registry_; }
  const PCG::OperationRegistry& GetOperationRegistry() const {
    return operation_registry_;
  }

private:
  PCG::OperationRegistry operation_registry_;
  std::unique_ptr<ValidationLogger> validation_logger_;
  std::unordered_map<std::string, ResourceID> procmodel_cache_;
};

}  // namespace ProcModel

#endif  // PROCMODEL_H