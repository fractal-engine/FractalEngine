#ifndef SCENE_VIEW_PIPELINE_H
#define SCENE_VIEW_PIPELINE_H

#include <bgfx/bgfx.h>

#include "editor/pipelines/scene_view_forward_pass.h"
#include "engine/ecs/ecs_collection.h"
#include "engine/renderer/frame_graph.h"

class SceneViewPipeline {
public:
  SceneViewPipeline();
  void Create();
  void Destroy();
  void Render();
  void RealRender();  // PLACEHOLDER: Remove this once pipeline is done

  void CreateFrameGraph();
  void RenderScenePass(const Pass::Context& ctx);

  // void SetSelectedEntity(EntityContainer* selected);

  // wireframe option
  bool wireframe_;

private:
  // Creates all passes
  /* void CreatePasses();

  // Destroys all passes
  void DestroyPasses();*/

  // Forward pass for scene view
  SceneViewForwardPass scene_view_forward_pass_;

  bgfx::ProgramHandle program_ BGFX_INVALID_HANDLE;

  // Entity selection
  std::vector<EntityContainer*> selected_entities;

  uint32_t msaa_samples_;
  bool frame_initialized_;
};

#endif  // SCENE_VIEW_PIPELINE_H