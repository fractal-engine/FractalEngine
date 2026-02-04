#include "node_types.h"

#include <FastNoise/FastNoise.h>

#include "../noise/OpenSimplex2S.hpp"  // ! check if it should use hpp or cpp
#include "../operators/fbm.h"
#include "../operators/remap.h"
#include "../operators/ridge.h"

namespace PCG {

NodeTypeDB& NodeTypeDB::Instance() {
  static NodeTypeDB db;
  return db;
}

NodeTypeDB::NodeTypeDB() {
  RegisterAllNodes();

  // Build name->id lookup
  for (size_t i = 0; i < _types.size(); ++i) {
    if (!_types[i].name.empty()) {
      _name_to_id[_types[i].name] = static_cast<NodeTypeID>(i);
    }
  }
}

void NodeTypeDB::RegisterAllNodes() {
  RegisterInputNodes();
  RegisterOutputNodes();
  RegisterMathNodes();
  RegisterNoiseNodes();
  RegisterFilterNodes();
  RegisterErosionNodes();
  RegisterUtilityNodes();
}

void NodeTypeDB::ForEachType(
    const std::function<void(uint32_t, const NodeType&)>& func) const {
  for (size_t i = 0; i < _types.size(); ++i) {
    if (!_types[i].name.empty()) {
      func(static_cast<uint32_t>(i), _types[i]);
    }
  }
}

//=============================================================================
// INPUT NODES
//=============================================================================
void NodeTypeDB::RegisterInputNodes() {
  {
    NodeType& t = _types[static_cast<size_t>(NodeTypeID::InputX)];
    t.name = "Input X";
    t.category = Category::Input;
    t.outputs = {{"x"}};
    t.process_func = [](ProcessContext& ctx) {
      ctx.set_output(0, ctx.position().x);
    };
  }
  {
    NodeType& t = _types[static_cast<size_t>(NodeTypeID::InputY)];
    t.name = "Input Y";
    t.category = Category::Input;
    t.outputs = {{"y"}};
    t.process_func = [](ProcessContext& ctx) {
      ctx.set_output(0, ctx.position().y);
    };
  }
  {
    NodeType& t = _types[static_cast<size_t>(NodeTypeID::Constant)];
    t.name = "Constant";
    t.category = Category::Input;
    t.outputs = {{"value"}};
    t.params = {{"value", NodeType::Param::Type::Float, 0.0f}};
    t.process_func = [](ProcessContext& ctx) {
      ctx.set_output(0, ctx.get_param<float>(0));
    };
  }
}

//=============================================================================
// NOISE NODES - custom implementations + FastNoise2
//=============================================================================
void NodeTypeDB::RegisterNoiseNodes() {
  // Custom OpenSimplex2 with derivatives
  {
    NodeType& t = _types[static_cast<size_t>(NodeTypeID::OpenSimplex2_Deriv)];
    t.name = "OpenSimplex2 (Derivatives)";
    t.category = Category::Noise;
    t.inputs = {{"x", 0.0f}, {"y", 0.0f}};
    t.outputs = {{"value"}, {"dx"}, {"dy"}};
    t.params = {
        {"seed", NodeType::Param::Type::Int, 12345},
        {"frequency", NodeType::Param::Type::Float, 0.01f, 0.001f, 1.0f, true}};
    t.process_func = [](ProcessContext& ctx) {
      auto* simplex = ctx.get_or_create_resource<OpenSimplex2S>(
          "simplex", ctx.get_param<int>(0));

      float freq = ctx.get_param<float>(1);
      float x = ctx.get_input(0) * freq;
      float y = ctx.get_input(1) * freq;

      double value, dx, dy;
      simplex->noise2_deriv(x, y, value, dx, dy);

      ctx.set_output(0, static_cast<float>(value));
      ctx.set_output(1, static_cast<float>(dx) * freq);
      ctx.set_output(2, static_cast<float>(dy) * freq);
    };
  }

  // Uber FBM with with erosion pipeline
  {
    NodeType& t = _types[static_cast<size_t>(NodeTypeID::UberFBM)];
    t.name = "Uber FBM";
    t.category = Category::Noise;
    t.inputs = {{"x", 0.0f}, {"y", 0.0f}};
    t.outputs = {{"value"}, {"dx"}, {"dy"}};
    t.params = {
        {"seed", NodeType::Param::Type::Int, 12345},
        {"octaves", NodeType::Param::Type::Int, 6, 1, 12, true},
        {"frequency", NodeType::Param::Type::Float, 0.01f, 0.001f, 1.0f, true},
        {"lacunarity", NodeType::Param::Type::Float, 2.0f, 1.0f, 4.0f, true},
        {"gain", NodeType::Param::Type::Float, 0.5f, 0.0f, 1.0f, true},
        {"sharpness", NodeType::Param::Type::Float, 0.0f, -1.0f, 1.0f, true},
        {"slope_erosion", NodeType::Param::Type::Float, 0.0f, 0.0f, 1.0f, true},
        {"ridge_erosion", NodeType::Param::Type::Float, 0.0f, 0.0f, 1.0f, true},
        {"altitude_erosion", NodeType::Param::Type::Float, 0.0f, 0.0f, 1.0f,
         true},
        {"perturb", NodeType::Param::Type::Float, 0.0f, 0.0f, 2.0f, true}};
    t.process_func = [](ProcessContext& ctx) {
      // Get or create UberFBM instance
      int seed = ctx.get_param<int>(0);
      auto* uber = ctx.get_or_create_resource<PCG::UberFBM>("uber_fbm", seed);

      PCG::UberFBMParams params;
      params.octaves = ctx.get_param<int>(1);
      params.lacunarity = ctx.get_param<float>(3);
      params.gain = ctx.get_param<float>(4);
      params.sharpness = ctx.get_param<float>(5);
      params.slope_erosion = ctx.get_param<float>(6);
      params.ridge_erosion = ctx.get_param<float>(7);
      params.altitude_erosion = ctx.get_param<float>(8);
      params.perturb = ctx.get_param<float>(9);

      float freq = ctx.get_param<float>(2);
      float x = ctx.get_input(0) * freq;
      float y = ctx.get_input(1) * freq;

      auto result = uber->Eval(x, y, ctx.get_param<int>(0), params);

      ctx.set_output(0, result.value);
      ctx.set_output(1, result.derivative.x);
      ctx.set_output(2, result.derivative.y);
    };
  }

  // FastNoise2 Simplex
  {
    NodeType& t = _types[static_cast<size_t>(NodeTypeID::FN2_Simplex2D)];
    t.name = "FN2 Simplex 2D";
    t.category = Category::Noise;
    t.inputs = {{"x", 0.0f}, {"y", 0.0f}};
    t.outputs = {{"value"}};
    t.params = {
        {"seed", NodeType::Param::Type::Int, 12345},
        {"frequency", NodeType::Param::Type::Float, 0.01f, 0.001f, 1.0f, true}};
    t.process_func = [](ProcessContext& ctx) {
      // Get or create the generator
      struct SimplexHolder {
        FastNoise::SmartNode<FastNoise::Simplex> node =
            FastNoise::New<FastNoise::Simplex>();
      };
      auto* holder = ctx.get_or_create_resource<SimplexHolder>("fn2_simplex");

      float freq = ctx.get_param<float>(1);
      float x = ctx.get_input(0) * freq;
      float y = ctx.get_input(1) * freq;

      // GenSingle2D(x, y, seed) - call through the SmartNode
      ctx.set_output(0, holder->node->GenSingle2D(x, y, ctx.get_param<int>(0)));
    };
  }
}

//=============================================================================
// FILTER NODES - Ridge, Terrace, Plateau
//=============================================================================
void NodeTypeDB::RegisterFilterNodes() {
  // Ridge operator
  {
    NodeType& t = _types[static_cast<size_t>(NodeTypeID::Ridge)];
    t.name = "Ridge/Billow";
    t.category = Category::Filter;
    t.inputs = {{"value", 0.0f}};
    t.outputs = {{"out"}};
    t.params = {
        {"sharpness", NodeType::Param::Type::Float, 0.0f, -1.0f, 1.0f, true}};
    t.process_func = [](ProcessContext& ctx) {
      float value = ctx.get_input(0);
      float sharpness = ctx.get_param<float>(0);
      ctx.set_output(0, PCG::Ridge::Blend(value, sharpness));
    };
  }

  // Terrace operator
  {
    NodeType& t = _types[static_cast<size_t>(NodeTypeID::Terrace)];
    t.name = "Terrace";
    t.category = Category::Filter;
    t.inputs = {{"value", 0.0f}};
    t.outputs = {{"out"}};
    t.params = {
        {"steps", NodeType::Param::Type::Int, 8, 1, 32, true},
        {"smoothness", NodeType::Param::Type::Float, 0.5f, 0.0f, 1.0f, true}};
    t.process_func = [](ProcessContext& ctx) {
      PCG::TerracingParams params;
      params.steps = ctx.get_param<int>(0);
      params.smoothness = ctx.get_param<float>(1);
      ctx.set_output(0, PCG::Remap::Terrace(ctx.get_input(0), params));
    };
  }

  // Plateau operator
  {
    NodeType& t = _types[static_cast<size_t>(NodeTypeID::Plateau)];
    t.name = "Plateau";
    t.category = Category::Filter;
    t.inputs = {{"value", 0.0f}};
    t.outputs = {{"out"}};
    t.params = {
        {"threshold", NodeType::Param::Type::Float, 0.5f, 0.0f, 1.0f, true},
        {"smoothness", NodeType::Param::Type::Float, 0.1f, 0.0f, 0.5f, true}};
    t.process_func = [](ProcessContext& ctx) {
      PCG::PlateauParams params;
      params.threshold = ctx.get_param<float>(0);
      params.smoothness = ctx.get_param<float>(1);
      ctx.set_output(0, PCG::Remap::Plateau(ctx.get_input(0), params));
    };
  }
}

//=============================================================================
// MATH NODES
//=============================================================================
void NodeTypeDB::RegisterMathNodes() {
  {
    NodeType& t = _types[static_cast<size_t>(NodeTypeID::Add)];
    t.name = "Add";
    t.category = Category::Math;
    t.inputs = {{"a", 0.0f}, {"b", 0.0f}};
    t.outputs = {{"out"}};
    t.process_func = [](ProcessContext& ctx) {
      ctx.set_output(0, ctx.get_input(0) + ctx.get_input(1));
    };
  }
  {
    NodeType& t = _types[static_cast<size_t>(NodeTypeID::Multiply)];
    t.name = "Multiply";
    t.category = Category::Math;
    t.inputs = {{"a", 0.0f}, {"b", 1.0f}};
    t.outputs = {{"out"}};
    t.process_func = [](ProcessContext& ctx) {
      ctx.set_output(0, ctx.get_input(0) * ctx.get_input(1));
    };
  }
  {
    NodeType& t = _types[static_cast<size_t>(NodeTypeID::Clamp)];
    t.name = "Clamp";
    t.category = Category::Math;
    t.inputs = {{"value", 0.0f}, {"min", 0.0f}, {"max", 1.0f}};
    t.outputs = {{"out"}};
    t.process_func = [](ProcessContext& ctx) {
      float v = ctx.get_input(0);
      float lo = ctx.get_input(1);
      float hi = ctx.get_input(2);
      ctx.set_output(0, std::clamp(v, lo, hi));
    };
  }
  {
    NodeType& t = _types[static_cast<size_t>(NodeTypeID::Mix)];
    t.name = "Mix (Lerp)";
    t.category = Category::Math;
    t.inputs = {{"a", 0.0f}, {"b", 1.0f}, {"t", 0.5f}};
    t.outputs = {{"out"}};
    t.process_func = [](ProcessContext& ctx) {
      float a = ctx.get_input(0);
      float b = ctx.get_input(1);
      float t = ctx.get_input(2);
      ctx.set_output(0, a + (b - a) * t);
    };
  }
}

// TODO: RegisterUtilityNodes, RegisterErosionNodes, RegisterOutputNodes

}  // namespace PCG