#include "generator_base.h"
#include "graph/generator_graph.h"

namespace PCG {

std::unique_ptr<GeneratorBase> GeneratorBase::Clone() const {
  return nullptr;
}

std::unique_ptr<GeneratorBase> CreateGenerator(GeneratorType type) {
  switch (type) {
    case GeneratorType::Graph:
      return std::make_unique<GeneratorGraph>();
    case GeneratorType::Preset:
      return nullptr;  // TODO: implement when PresetGenerator exists
    default:
      return nullptr;
  }
}

}  // namespace PCG