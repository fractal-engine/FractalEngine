#ifndef SCENE_VIEW_PIPELINE_H
#define SCENE_VIEW_PIPELINE_H

#include <bgfx/bgfx.h>

#include "editor/pipelines/scene_view_forward_pass.h"
#include "engine/ecs/ecs_collection.h"
#include "editor/gizmos/scene_view_gizmo.h"

class SceneViewPipeline {
public:
  SceneViewPipeline();
  void Create();
  void Destroy();
  void Render();
  void RealRender();  // PLACEHOLDER: Remove this once pipeline is done

  // wireframe option
  bool wireframe_;

private:
  // Forward pass for scene view
  SceneViewForwardPass scene_view_forward_pass_;
  SceneViewGizmo m_scene_view_gizmo;
  bgfx::ProgramHandle program_ BGFX_INVALID_HANDLE;

  // Entity selection
  std::vector<EntityContainer*> selected_entities;

  uint32_t msaa_samples_;
  bool frame_initialized_;
};

#endif  // SCENE_VIEW_PIPELINE_H