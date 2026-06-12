#ifndef PROCMODEL_VALIDATOR_H
#define PROCMODEL_VALIDATOR_H

#include "engine/pcg/procmodel/descriptor/model_descriptor.h"
#include "engine/pcg/procmodel/generator/resolved_model.h"
#include "engine/pcg/procmodel/model_graph/model_graph.h"
#include "engine/pcg/procmodel/validation/parameter_sampler.h"

#include "procmodel_analysis.h"

namespace ProcModel {

// TODO: Split ProcModelValidator telemetry into a separate ProcModelInspector
// - telemetry: ComputeBounds, RecordRawData, CheckForwardAxisConsistency

// Post-generation validator for ProcModel instances.
//
// Runs a fixed set of structural and geometric checks over a InstanceModel.
// Each check may produce Diagnostic entries in the returned
// ProcModelSample. Overall pass/fail is determined by whether any
// Severity::Error diagnostic was emitted.
//
// Called by ModelGenerator::Generate after ValidateConstraints succeeds.
// If the result is not passed, the generator retries with the next seed.
//
// Stateless and thread-safe: takes all context by const-ref, returns a new
// ProcModelSample per call. No caching, no configuration.
class ProcModelValidator {
public:
  static ProcModelSample Validate(const InstanceData& data,
                                  const ModelGraph& graph,
                                  const ModelDescriptor& descriptor,
                                  int attempt_index, ParameterSampler& sampler);
};

}  // namespace ProcModel

#endif  // PROCMODEL_VALIDATOR_H