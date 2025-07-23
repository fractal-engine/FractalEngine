#ifndef SCENE_VIEW_PIPELINE_H
#define SCENE_VIEW_PIPELINE_H

#include <bgfx/bgfx.h>
#include "engine/ecs/world.h"

class SceneViewPipeline {
public:
  SceneViewPipeline();
  void Create();
  void Destroy();
  void Render(bgfx::ViewId view, const float* view_mtx, const float* proj_mtx);

private:
  bgfx::ProgramHandle program_ BGFX_INVALID_HANDLE;

  bool wireframe_{false};
  int msaa_samples_{4};
  bool frame_initialized_{false};
};

#endif  // SCENE_VIEW_PIPELINE_H