#ifndef BIOME_PRESETS_H
#define BIOME_PRESETS_H

#include "constraint_system.h"

namespace Generator {

namespace BiomePresets {
// Temperate climate biome set
static void ApplyTemperateRules(ConstraintSystem& system) {
  // Lowland marker
  Rule water;
  water.constraints = {{"height", -10.0f, -0.5f}};
  water.outputs["color"] = {0.1f, 0.3f, 0.6f, 1.0f};  // Deep blue
  water.priority = 1;
  system.AddRule(water);

  Rule beach;
  beach.constraints = {{"height", -0.5f, 2.0f}};
  beach.outputs["color"] = {0.9f, 0.8f, 0.6f, 1.0f};  // Sand
  beach.priority = 2;
  system.AddRule(beach);

  // Grassland
  Rule grassland;
  grassland.constraints = {{"height", 2.0f, 25.0f}, {"slope", 0.0f, 0.6f}};
  grassland.outputs["color"] = {0.2f, 0.6f, 0.2f, 1.0f};  // Green
  grassland.priority = 3;
  system.AddRule(grassland);

  // Forest
  Rule forest;
  forest.constraints = {{"height", 10.0f, 30.0f}, {"slope", 0.3f, 0.8f}};
  forest.outputs["color"] = {0.1f, 0.4f, 0.1f, 1.0f};  // Dark green
  forest.priority = 4;
  system.AddRule(forest);

  // Rocky slopes
  Rule rocky;
  rocky.constraints = {{"height", 15.0f, 45.0f}, {"slope", 0.8f, 999.0f}};
  rocky.outputs["color"] = {0.5f, 0.5f, 0.5f, 1.0f};  // Gray
  rocky.priority = 5;
  system.AddRule(rocky);

  // Mountain peaks
  Rule mountain;
  mountain.constraints = {{"height", 35.0f, 999.0f}};
  mountain.outputs["color"] = {0.9f, 0.9f, 0.9f, 1.0f};  // Snow white
  mountain.priority = 6;                                 // Highest priority
  system.AddRule(mountain);
}

// Desert climate biome set
static void ApplyDesertRules(ConstraintSystem& system) {
  Rule sand_dunes;
  sand_dunes.constraints = {{"height", 0.0f, 20.0f}, {"slope", 0.0f, 0.5f}};
  sand_dunes.outputs["color"] = {0.9f, 0.75f, 0.5f, 1.0f};  // Sandy
  sand_dunes.priority = 1;
  system.AddRule(sand_dunes);

  Rule rocky_desert;
  rocky_desert.constraints = {{"height", 10.0f, 40.0f},
                              {"slope", 0.5f, 999.0f}};
  rocky_desert.outputs["color"] = {0.7f, 0.5f, 0.3f, 1.0f};  // Brown rock
  rocky_desert.priority = 2;
  system.AddRule(rocky_desert);

  // TODO: add rules here
}

// Arctic climate
static void ApplyArcticRules(ConstraintSystem& system) {
  Rule snow;
  snow.constraints = {{"height", -999.0f, 999.0f}};  // snow
  snow.outputs["color"] = {0.95f, 0.95f, 0.98f, 1.0f};
  snow.priority = 1;
  system.AddRule(snow);
}
}  // namespace BiomePresets

}  // namespace Generator

#endif  // BIOME_PRESETS_H