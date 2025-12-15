#ifndef GAME_VIEW_PIPELINE_H
#define GAME_VIEW_PIPELINE_H

#include <bgfx/bgfx.h>

#include "engine/ecs/ecs_collection.h"
#include "engine/renderer/frame_graph.h"

class GameViewPipeline {

public:
  GameViewPipeline();
  void Create();
  void Destroy();
  void Render();

private:



}

#endif  // GAME_VIEW_PIPELINE_H
