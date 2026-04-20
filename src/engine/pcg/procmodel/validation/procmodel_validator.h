#ifndef PROCMODEL_VALIDATOR_H
#define PROCMODEL_VALIDATOR_H

#include "engine/pcg/procmodel/descriptor/model_descriptor.h"
#include "engine/pcg/procmodel/generator/resolved_model.h"
#include "engine/pcg/procmodel/model_graph/model_graph.h"

#include "validation_result.h"

namespace ProcModel {

// Post-generation validator for ProcModel instances.
//
// Runs a fixed set of structural and geometric checks over a ResolvedModel.
// Each check may produce Diagnostic entries in the returned
// ValidationResult. Overall pass/fail is determined by whether any
// Severity::Error diagnostic was emitted.
//
// Called by ModelGenerator::Generate after ValidateConstraints succeeds.
// If the result is not passed, the generator retries with the next seed.
//
// Stateless and thread-safe: takes all context by const-ref, returns a new
// ValidationResult per call. No caching, no configuration.
class ProcModelValidator {
public:
  static ValidationResult Validate(const ResolvedModel& resolved,
                                   const ModelGraph& graph,
                                   const ModelDescriptor& descriptor,
                                   int attempt_index);
};

}  // namespace ProcModel

#endif  // PROCMODEL_VALIDATOR_H