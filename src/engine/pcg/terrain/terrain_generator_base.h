#ifndef TERRAIN_GENERATOR_BASE_H
#define TERRAIN_GENERATOR_BASE_H

#include <glm/glm.hpp>
#include <memory>
#include <string>

#include "../core/sample.h"

namespace PCG {

// Forward declaration
class ProgramGraph;

//=============================================================================
// GENERATOR TYPE
//=============================================================================
enum class GeneratorType {
  Preset,  // PCG::Generator with Config
  Graph,   // Node graph (uses ProgramGraph)
};

inline const char* GetGeneratorTypeName(GeneratorType type) {
  switch (type) {
    case GeneratorType::Preset:
      return "Preset";
    case GeneratorType::Graph:
      return "Graph";
    default:
      return "Unknown";
  }
}

//=============================================================================
// GENERATOR BASE - Interface
//=============================================================================
class GeneratorBase {
public:
  virtual ~GeneratorBase() = default;

  virtual GeneratorType GetType() = 0;
  virtual std::string GetDisplayName() = 0;

  // Point evaluation
  virtual Sample Eval(float x, float y) = 0;

  // Graph access (Graph type only)
  virtual ProgramGraph* GetGraph() { return nullptr; }
  virtual const ProgramGraph* GetGraph() const { return nullptr; }

  // Clone for copy operations
  virtual std::unique_ptr<GeneratorBase> Clone() = 0;
};

//=============================================================================
// FACTORY
//=============================================================================
std::unique_ptr<GeneratorBase> CreateGenerator(GeneratorType type);

}  // namespace PCG

#endif  // TERRAIN_GENERATOR_BASE_H