#ifndef SCENE_VIEW_PIPELINE_H
#define SCENE_VIEW_PIPELINE_H

#include <bgfx/bgfx.h>

#include "engine/ecs/ecs_collection.h"
#include "engine/renderer/frame_graph.h"
#include "engine/transform/transform_system.h"
#include "editor/gui/orbit_camera.h"
class SceneViewPipeline {
public:
  SceneViewPipeline();
  void Create();
  void Destroy();
  void Render();
  void RealRender();  // PLACEHOLDER: Remove this once pipeline is done

  void CreateFrameGraph();

  void RenderReflectionPass(const Pass::Context& ctx);
  void RenderForwardPass(const Pass::Context& ctx);
  void RenderSelectedEntity(EntityContainer* entity,
                            const glm::mat4& view_projection,
                            const OrbitCamera& camera);

  // void SetSelectedEntity(EntityContainer* selected);

  // wireframe option
  bool wireframe_;

  bool show_skybox_;

  bool show_terrain_;

  bool show_water_;

  bool show_gizmos_;

  bool render_shadows_;

private:
  // Creates all passes
  /* void CreatePasses();

  // Destroys all passes
  void DestroyPasses();*/

  // Forward pass for scene view
  // SceneViewForwardPass scene_view_forward_pass_;

  // Link systems
  TransformSystem transform_system;

  // Cache
  bgfx::ProgramHandle program_ BGFX_INVALID_HANDLE;

  // Entity selection
  std::vector<EntityContainer*> selected_entities;

  // TODO: replace with actual implementation of selection material
  bgfx::ProgramHandle selection_material_ BGFX_INVALID_HANDLE;  // placeholder

  uint32_t msaa_samples_;
  bool frame_initialized_;
};

#endif  // SCENE_VIEW_PIPELINE_H