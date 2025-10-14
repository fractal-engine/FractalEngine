#ifndef SCENE_VIEW_PIPELINE_H
#define SCENE_VIEW_PIPELINE_H

#include <bgfx/bgfx.h>

#include "engine/ecs/ecs_collection.h"
#include "engine/renderer/frame_graph.h"
#include "engine/transform/transform_system.h"
class SceneViewPipeline {
public:
  SceneViewPipeline();

  void Create();
  void Destroy();
  void Render();

  // void RealRender();  // PLACEHOLDER: Remove this once pipeline is done

  Camera& GetGodCamera();

  // void CreateFrameGraph();

  // void RenderSelectedEntity(EntityContainer* entity, const glm::mat4&
  // view_projection, const SceneCamera& camera);

  // void SetSelectedEntity(EntityContainer* selected);

  // Pipeline settings
  void SetWireframe(bool enabled) { wireframe_ = enabled; }
  void SetShowSkybox(bool enabled) { show_skybox_ = enabled; }
  void SetShowGizmos(bool enabled) { show_gizmos_ = enabled; }
  void SetSelectedEntities(const std::vector<EntityContainer*>& entities) {
    selected_entities_ = entities;
  }

  // Settings
  bool wireframe_;
  bool show_skybox_;
  bool show_terrain_;
  bool show_water_;
  bool show_gizmos_;
  bool render_shadows_;
  uint32_t msaa_samples_;

private:
  // Creates all passes
  /* void CreatePasses();

  // Destroys all passes
  void DestroyPasses();*/

  // Forward pass for scene view
  // SceneViewForwardPass scene_view_forward_pass_;

  // Register nodes with FrameGraph
  void RegisterNodes();

  void RenderReflectionNode(const Node::Context& context);
  void RenderForwardNode(const Node::Context& context);
  void RenderSelectionOutlineNode(const Node::Context& context);
  void RenderGizmosNode(const Node::Context& context);

  // Helper
  void RenderSelectedEntityOutline(EntityContainer* entity,
                                   const glm::mat4& view_projection);

  // Camera
  TransformComponent god_camera_transform_;
  CameraComponent god_camera_root_;
  Camera god_camera_;

  // Transform system
  TransformSystem transform_system_;

  // Cache
  // bgfx::ProgramHandle program_ BGFX_INVALID_HANDLE;

  // Entity selection
  std::vector<EntityContainer*> selected_entities_;

  // TODO: replace with actual implementation of selection material
  // bgfx::ProgramHandle selection_material_ BGFX_INVALID_HANDLE;  //
  // placeholder

  // bool frame_initialized_;

  // Shader handles (loaded once)
  bgfx::ProgramHandle gltf_program_;
  bgfx::ProgramHandle selection_program_;
};

#endif  // SCENE_VIEW_PIPELINE_H