#ifndef BIOME_PRESETS_H
#define BIOME_PRESETS_H

#include "constraint_system.h"

namespace PCG {

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

  // Lowlands - Dark green
  Rule lowlands;
  lowlands.constraints = {{"height", 2.0f, 6.0f}, {"slope", 0.0f, 0.20f}};
  lowlands.outputs["color"] = {0.15f, 0.35f, 0.12f, 1.0f};  // Dark swamp green
  lowlands.priority = 15;
  system.AddRule(lowlands);

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

  // Rocky slopes - DARK BROWN ROCKS
  Rule rocky_slopes;
  rocky_slopes.constraints = {
      {"height", 6.0f, 20.0f}, {"slope", 0.45f, 1.0f}
      // Steep slopes = exposed rock
  };
  rocky_slopes.outputs["color"] = {0.35f, 0.25f, 0.15f, 1.0f};  // DARK BROWN
  rocky_slopes.priority = 30;  // Higher priority than grass/forest
  system.AddRule(rocky_slopes);

  // Mountain ridges - ORANGE-BROWN ROCK
  Rule mountain_ridges;
  mountain_ridges.constraints = {
      {"height", 15.0f, 30.0f}, {"slope", 0.40f, 1.0f}  // Steep ridges
  };
  mountain_ridges.outputs["color"] = {0.55f, 0.40f, 0.25f,
                                      1.0f};  // Orange-brown
  mountain_ridges.priority = 35;
  system.AddRule(mountain_ridges);

  // Cliff faces - GRAY ROCK
  Rule cliffs;
  cliffs.constraints = {
      {"height", 25.0f, 40.0f}, {"slope", 0.60f, 1.0f}
      // Very steep = sheer cliffs
  };
  cliffs.outputs["color"] = {0.45f, 0.45f, 0.45f, 1.0f};  // Gray stone
  cliffs.priority = 45;
  system.AddRule(cliffs);

  // Mountain peaks
  Rule mountain;
  mountain.constraints = {{"height", 35.0f, 999.0f}};
  mountain.outputs["color"] = {0.9f, 0.9f, 0.9f, 1.0f};  // Snow white
  mountain.priority = 6;                                 // Highest priority
  system.AddRule(mountain);
}

// Desert climate biome set
static void ApplyDesertRules(ConstraintSystem& system) {
  // Sand dunes - Warm orange-brown sand
  Rule sand_dunes;
  sand_dunes.constraints = {{"height", 0.0f, 20.0f}, {"slope", 0.0f, 0.5f}};
  sand_dunes.outputs["color"] = {0.85f, 0.65f, 0.40f, 1.0f};  // Orange sand
  sand_dunes.priority = 1;
  system.AddRule(sand_dunes);

  // Rocky desert - Dark orange-brown rock
  Rule rocky_desert;
  rocky_desert.constraints = {{"height", 10.0f, 40.0f},
                              {"slope", 0.5f, 999.0f}};
  rocky_desert.outputs["color"] = {0.60f, 0.40f, 0.25f, 1.0f};  // orange-brown
  rocky_desert.priority = 2;
  system.AddRule(rocky_desert);

  // Desert scrubland - Dry grass
  Rule scrubland;
  scrubland.constraints = {{"height", 0.0f, 15.0f}, {"slope", 0.0f, 0.3f}};
  scrubland.outputs["color"] = {0.70f, 0.60f, 0.35f, 1.0f};  // yellow-brown
  scrubland.priority = 3;
  system.AddRule(scrubland);
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

}  // namespace PCG

#endif  // BIOME_PRESETS_H