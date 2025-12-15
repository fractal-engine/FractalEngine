#ifndef NODE_TYPES_H
#define NODE_TYPES_H

#include <array>
#include <functional>
#include <string>
#include <variant>
#include <vector>

namespace PCG {

// Forward declarations
struct ProcessContext;
struct RangeAnalysisContext;

//=============================================================================
// NODE TYPE DEFINITIONS
//=============================================================================
enum class Category {
  Input,
  Output,
  Math,
  Noise,
  Filter,   // operators
  Erosion,  // Altitude/Slope/Ridge erosion
  Utility
};

struct NodeType {
  struct Port {
    std::string name;
    float default_value = 0.0f;
  };

  struct Param {
    enum class Type { Float, Int, Enum, Bool };
    std::string name;
    Type type;
    std::variant<float, int, bool, std::string> default_value;
    float min_value = 0.0f;
    float max_value = 1.0f;
    bool has_range = false;
    std::vector<std::string> enum_values;  // For enum types
  };

  std::string name;
  Category category;
  std::vector<Port> inputs;
  std::vector<Port> outputs;
  std::vector<Param> params;

  // Processing functions
  std::function<void(ProcessContext&)> process_func;
  std::function<void(RangeAnalysisContext&)> range_func;
};

//=============================================================================
// NODE TYPE IDS - terrain generation nodes
//=============================================================================

enum class NodeTypeID : uint32_t {
  // Inputs
  InputX = 0,
  InputY,
  InputZ,
  Constant,

  // Outputs
  OutputHeight,
  OutputSlope,

  // Math operations
  Add,
  Subtract,
  Multiply,
  Divide,
  Clamp,
  Mix,  // Lerp
  Abs,
  Power,
  Min,
  Max,

  // Noise generators
  OpenSimplex2,        // OpenSimplex2S
  OpenSimplex2_Deriv,  // analytical derivatives
  UberFBM,             // custom fBm with erosion

  // Noise generators - FastNoise2
  FN2_Simplex2D,
  FN2_Simplex3D,
  FN2_Perlin2D,
  FN2_Perlin3D,
  FN2_Cellular2D,
  FN2_Value2D,

  // Domain operations
  DomainWarp,
  DomainWarpFractal,  // Progressive fractal warp

  // Filters - operators
  Ridge,    // Ridge::Blend
  Terrace,  // Remap::Terrace
  Plateau,  // Remap::Plateau

  // Erosion - erosion system
  SlopeErosion,
  RidgeErosion,
  AltitudeErosion,

  // Utility
  Preview,  // SDF preview node
  Comment,

  COUNT
};

//=============================================================================
// NODE TYPE DATABASE
//=============================================================================
class NodeTypeDB {
public:
  static NodeTypeDB& Instance();

  const NodeType& Get(NodeTypeID id) const;
  const NodeType& Get(uint32_t id) const;

  // Lookup by name (for serialization)
  bool TryGetId(const std::string& name, NodeTypeID& out_id) const;

private:
  NodeTypeDB();
  void RegisterAllNodes();

  // Registration helpers - grouped by category
  void RegisterInputNodes();
  void RegisterOutputNodes();
  void RegisterMathNodes();
  void RegisterNoiseNodes();    // Both custom AND FastNoise2
  void RegisterFilterNodes();   // Ridge, Terrace, Plateau
  void RegisterErosionNodes();  // Your erosion system
  void RegisterUtilityNodes();

  std::array<NodeType, static_cast<size_t>(NodeTypeID::COUNT)> _types;
  std::unordered_map<std::string, NodeTypeID> _name_to_id;
};

}  // namespace PCG

#endif  // NODE_TYPES_H