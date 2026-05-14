#ifndef PIPELINE_DESCRIPTOR_PARSER_H
#define PIPELINE_DESCRIPTOR_PARSER_H

#include <nlohmann/json_fwd.hpp>

namespace PCG {

struct PipelineDescriptor;

// Parses a "pipeline" array from a JSON object into out.
// Missing or non-array "pipeline" fields are valid
// Per-entry validation failures log warnings and skip the entry.
// Returns false only on unrecoverable parser errors.
bool ParsePipelineDescriptor(const nlohmann::json& j, PipelineDescriptor& out);

}  // namespace PCG

#endif  // PIPELINE_DESCRIPTOR_PARSER_H