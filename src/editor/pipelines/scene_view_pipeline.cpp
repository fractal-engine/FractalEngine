#include "scene_view_pipeline.h"

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "editor/editor_ui.h"
#include "editor/gizmos/component_gizmos.h"
#include "editor/runtime/runtime.h"  // TODO: Remove this once pipeline is done
#include "engine/core/engine_globals.h"
#include "engine/core/logger.h"
#include "engine/core/view_ids.h"
#include "engine/ecs/world.h"
#include "engine/renderer/model/mesh.h"

SceneViewPipeline::SceneViewPipeline()
    : wireframe_(false), msaa_samples_(4), frame_initialized_(false) {
  // TODO: Initialize other members e.g. profile, camera, viewport, passes, etc.
  // Check reference project for details
}

void SceneViewPipeline::CreateFrameGraph() {
  // Get reference to the FrameGraph owned by Runtime
  auto& frameGraph = Runtime::GetFrameGraph();

  // Clear any existing passes and attachments
  frameGraph.Clear();

  // Define scene attachments
  AttachmentDesc colorAttachment{
      .name = "scene_color",
      .width = 0,   // Will be updated during Rebuild()
      .height = 0   // Will be updated during Rebuild()
  };

  AttachmentDesc depthAttachment{
      .name = "scene_depth",
      .width = 0,   // Will be updated during Rebuild()
      .height = 0   // Will be updated during Rebuild()
  };

  // Add attachments to frame graph
  frameGraph.AddAttachment(colorAttachment);
  frameGraph.AddAttachment(depthAttachment);

  // Define scene rendering pass
  Pass scenePass{.name = "scene_forward_pass",
                             .execute =
                                 [this](const Pass::Context& ctx) {
                                   // This lambda contains all the rendering
                                   // logic that was previously in Render()
                                   this->RenderScenePass(ctx);
                                 },
                             .reads = {},
                             .writes = {"scene_color", "scene_depth"}};

  // Add pass to frame graph
  frameGraph.AddPass(scenePass);

  // Bake frame graph to finalize
  frameGraph.Bake();

  Logger::getInstance().Log(LogLevel::Info,
                            "SceneViewPipeline: Frame graph created");
}

void SceneViewPipeline::RenderScenePass(
    const Pass::Context& /*ctx*/) {

  auto& world = ECS::Main();

  // Orbit camera from editor
  OrbitCamera& camera = EditorUI::Get()->GetCamera();

  // Calculate the LIVE view and projection matrices FROM THE ORBIT CAMERA.
  float viewMatrix[16];
  float projMatrix[16];
  camera.getViewMatrix(viewMatrix);
  camera.getProjectionMatrix(projMatrix,
                             float(canvasViewportW) / float(canvasViewportH));

  // View setup for scene using the matrices from the OrbitCamera
  bgfx::setViewTransform(ViewID::SCENE_MESH, viewMatrix, projMatrix);
  bgfx::setViewRect(ViewID::SCENE_MESH, 0, 0, canvasViewportW, canvasViewportH);
  bgfx::setViewClear(ViewID::SCENE_MESH, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                     0x303030ff, 1.0f, 0);

  // Update transforms and draw
  world.UpdateTransforms();
  const auto& render_queue = world.GetRenderQueue();

  // Render all the meshes in the scene.
  for (const auto& [entity, transform, renderer] : render_queue) {
    if (!renderer.enabled_ || !renderer.mesh_)
      continue;

    bgfx::setTransform(glm::value_ptr(transform.model_));
    renderer.mesh_->Bind();

    bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_CULL_CW);
    bgfx::submit(ViewID::SCENE_MESH,
                 Runtime::Shader()->LoadProgram("gltf_default", "vs_gltf.bin",
                                                "fs_gltf.bin"));
  }

  // Get the selected entity from the UI singleton.
  Entity selectedEntity = EditorUI::Get()->GetSelectedEntity();

  // Draw the gizmo ONCE, after all meshes have been rendered.
  ComponentGizmos::DrawTransformGizmo(selectedEntity, viewMatrix, projMatrix);
}

void SceneViewPipeline::Create() {
  // TODO: Initialize default profile (exposure, contrast, gamma, etc)

  // TODO: set up fly camera for scene view

  // Build frame graph
  CreateFrameGraph();

  // TODO: set future game view for gizmos
  // Use GameViewPipeline for the gizmos

  // TODO: remove this once the above is implemented
  if (bgfx::isValid(program_))
    return;
}

void SceneViewPipeline::Destroy() {

  // TODO: remove this once the above is implemented
  if (bgfx::isValid(program_))
    bgfx::destroy(program_), program_ = BGFX_INVALID_HANDLE;

  Logger::getInstance().Log(LogLevel::Debug,
                            "[Render] SceneViewPipeline::Destroy");
}

// TODO: rename to Render() once placeholder is removed
void SceneViewPipeline::RealRender() {
  // debug
  /*Logger::getInstance().Log(LogLevel::Debug,
                            "[Render] SceneViewPipeline::Render called");*/

  // Set default view and matrices
  /*bgfx::ViewId view = ViewID::SCENE_MESH;
  glm::mat4 view_mtx = glm::mat4(1.0f);
  glm::mat4 proj_mtx = glm::mat4(1.0f);*/

  // TODO: Start profiler ("scene_view")

  // TODO: Pick camera and get camera bindings
  // TODO: Select profile (default or game profile)
  // TODO: Compute view/projection matrices

  // TODO: start new gizmo frame

  // TRANSFORM PASS
  // TODO: Evaluate and update transforms

  // PRE PASS
  // TODO: Geometry/depth pass before forward pass

  // SSAO PASS
  // TODO: calculate screen space ambient occlusion if enabled

  // VELOCITY BUFFER PASS
  // TODO: Set velocity buffer to none

  // FORWARD PASS
  // TODO: Prepare material, lighting, shadow data
  // TODO: Set wireframe, skybox, gizmo flags here
  // TODO: Link skybox

  // TODO: remove this once all todos are implemented
  /* auto& world = ECS::Main();
  world.UpdateTransforms();
  const auto& render_queue = world.GetRenderQueue();

  Logger::getInstance().Log(
      LogLevel::Debug,
      "[Render] Render queue size: " + std::to_string(render_queue.size()));

  bgfx::setViewTransform(view, glm::value_ptr(view_mtx),
                         glm::value_ptr(proj_mtx));

  for (const auto& item : render_queue) {

    const auto& transform = std::get<1>(item);
    const auto& renderer = std::get<2>(item);

    Logger::getInstance().Log(
        LogLevel::Debug,
        "[Render] Entity: " +
            std::to_string(static_cast<uint32_t>(std::get<0>(item))) +
            " enabled: " + std::to_string(renderer.enabled_) + " mesh: " +
            std::to_string(reinterpret_cast<uintptr_t>(renderer.mesh_)));

    if (!renderer.enabled_ || !renderer.mesh_)
      continue;

    bgfx::setTransform(glm::value_ptr(transform.model_));
    renderer.mesh_->Bind();
    Logger::getInstance().Log(LogLevel::Debug,
                              "[Render] Mesh bound and submitted");
    bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_CULL_CW);
    bgfx::submit(view, program_);
  } */

  // POST PROCESSING PASS
  // TODO: Render post-processing effects here
  // use forward pass output as input

  // TODO: Stop profiler ("scene_view")
}

// PLACEHOLDER: Remove this once pipeline is done
void SceneViewPipeline::Render() {

  // TODO: add missing inits and calls noted in RealRender

  Runtime::GetFrameGraph().Render();
}

/*
void SceneViewPipeline::CreatePasses() {}

void SceneViewPipeline::DestroyPasses() {
  scene_view_forward_pass_.Destroy();
}*/