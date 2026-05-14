#ifndef PCG_PIPELINE_DESCRIPTOR_H
#define PCG_PIPELINE_DESCRIPTOR_H

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace PCG {

// Source form: one entry from the JSON "pipeline" array.
// Raw params are kept around for re-compilation on hot-reload.
struct PipelineEntry {
  std::string kind;
  nlohmann::json params;
};

// Source form: full pipeline as authored in JSON. Lives on subsystem-specific
// resources alongside compiled topology-specific pipeline
struct PipelineDescriptor {
  std::vector<PipelineEntry> entries;
};

}  // namespace PCG

#endif  // PCG_PIPELINE_DESCRIPTOR_H