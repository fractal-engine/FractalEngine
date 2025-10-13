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
#include "engine/renderer/lighting/light.h"
#include "engine/renderer/model/mesh.h"
#include "engine/transform/transform.h"

SceneViewPipeline::SceneViewPipeline()
    : wireframe_(false),
      show_skybox_(true),
      msaa_samples_(4),
      frame_initialized_(false),
      show_terrain_(true),
      show_water_(true),
      show_gizmos_(true),
      render_shadows_(true),
      program_(BGFX_INVALID_HANDLE),
      transform_system(),
      selected_entities(),
      selection_material_(BGFX_INVALID_HANDLE) {
  // TODO: Initialize other members e.g. profile, camera, viewport, passes, etc.
  // Check reference project for details
}

void SceneViewPipeline::CreateFrameGraph() {
  // Get reference to FrameGraph owned by Runtime
  auto& frame_graph = Runtime::GetFrameGraph();

  // Clear passes and attachments
  frame_graph.Clear();

  // Attachments
  AttachmentDesc colorAttachment{
      .name = "scene_color", .width = 0, .height = 0};

  AttachmentDesc depthAttachment{
      .name = "scene_depth", .width = 0, .height = 0};

  AttachmentDesc reflectionAttachment{
      .name = "reflection", .width = 0, .height = 0};

  // Add attachments to frame graph
  frame_graph.AddAttachment(colorAttachment);
  frame_graph.AddAttachment(depthAttachment);
  frame_graph.AddAttachment(reflectionAttachment);

  /* ---------- DEFINE PASSES ---------- */
  // Reflection pass
  Pass reflection_pass{
      .name = "reflection_pass",
      .execute =
          [this](const Pass::Context& ctx) { this->RenderReflectionPass(ctx); },
      .reads = {},
      .writes = {"reflection"}};
  frame_graph.AddPass(reflection_pass);

  // Mesh pass
  Pass forward_pass{
      .name = "forward_pass",
      .execute =
          [this](const Pass::Context& ctx) { this->RenderForwardPass(ctx); },
      .reads = {},  // can sample reflection
      .writes = {"scene_color", "scene_depth"}};

  // Add pass to frame graph
  frame_graph.AddPass(forward_pass);

  // Bake frame graph to finalize
  frame_graph.Bake();

  Logger::getInstance().Log(LogLevel::Info,
                            "SceneViewPipeline: Frame graph created");
}

// TODO: implement into a larger standalone render pass (?)
// TODO: we already use OrbitCamera& camera in ForwardPass, maybe we should
// delete it here
void SceneViewPipeline::RenderReflectionPass(const Pass::Context& ctx) {
  // Get the GraphicsRenderer
  auto* graphics_renderer = static_cast<GraphicsRenderer*>(Runtime::Renderer());
  if (!graphics_renderer) {
    Logger::getInstance().Log(
        LogLevel::Error,
        "RenderReflectionPass: Failed to get GraphicsRenderer");
    return;
  }

  // Bind reflection target
  uint16_t fbw = graphics_renderer->GetFramebufferWidth();
  uint16_t fbh = graphics_renderer->GetFramebufferHeight();
  bgfx::setViewRect(ViewID::REFLECTION_PASS, 0, 0, fbw, fbh);
  bgfx::setViewFrameBuffer(ViewID::REFLECTION_PASS,
                           graphics_renderer->GetReflectionFramebuffer());
  bgfx::setViewClear(ViewID::REFLECTION_PASS,
                     BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);

  OrbitCamera& camera = EditorUI::Get()->GetCamera();
  float viewMatrix[16];
  float projMatrix[16];
  camera.getViewMatrix(viewMatrix);
  camera.getProjectionMatrix(projMatrix,
                             float(canvasViewportW) / float(canvasViewportH));

  // Build reflected view (Y-plane mirror)
  float reflect[16];
  bx::mtxIdentity(reflect);
  reflect[5] = -1.0f;  // flip Y
  float reflectedView[16];
  bx::mtxMul(reflectedView, viewMatrix, reflect);

  // Skybox in reflection (rotation-only)
  float rotOnly[16];
  memcpy(rotOnly, reflectedView, sizeof(rotOnly));
  rotOnly[12] = rotOnly[13] = rotOnly[14] = 0.0f;

  if (show_skybox_ && ctx.globals.skybox) {
    ctx.globals.skybox->Submit(ViewID::REFLECTION_PASS, rotOnly, projMatrix,
                               /*useInverseViewProj=*/false);
  }
}

void SceneViewPipeline::RenderForwardPass(const Pass::Context& ctx) {

  // TODO: In this function, we should:
  // Setup view
  // Update transforms
  // Render all scene meshes
  // Draw gizmos

  auto* graphics_renderer = static_cast<GraphicsRenderer*>(Runtime::Renderer());

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
  bgfx::setViewTransform(ViewID::SCENE_FORWARD, viewMatrix, projMatrix);
  bgfx::setViewRect(ViewID::SCENE_FORWARD, 0, 0, canvasViewportW,
                    canvasViewportH);
  bgfx::setViewClear(ViewID::SCENE_FORWARD, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                     0x303030ff, 1.0f, 0);

  const auto& render_queue = world.GetRenderQueue();

  // Build view_projection
  glm::mat4 view = glm::make_mat4(viewMatrix);
  glm::mat4 projection = glm::make_mat4(projMatrix);
  glm::mat4 view_projection = projection * view;

  // Update transforms and draw
  transform_system.Perform(view_projection);

  // Query directional light with ECS and set uniforms
  auto light_view = world.View<DirectionalLightComponent, TransformComponent>();

  for (auto [e, light, tr] : light_view.each()) {
    if (!light.enabled_)
      continue;

    glm::vec3 dir = glm::normalize(Transform::Forward(tr, Space::WORLD));
    Light::SetDirectionalLight(dir, light.color_, light.intensity_);
    break;
  }

  Light::ApplyUniforms();

  // Bulk MVP update
  for (auto& [entity, transform, renderer] : render_queue) {
    Transform::UpdateMVP(transform, view_projection);
  }

  // Render all the meshes in scene
  for (const auto& [entity, transform, renderer] : render_queue) {
    if (!renderer.enabled_ || !renderer.mesh_)
      continue;

    bgfx::setTransform(glm::value_ptr(transform.model_));
    renderer.mesh_->Bind();

    bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_CULL_CW);
    bgfx::submit(ViewID::SCENE_FORWARD,
                 Runtime::Shader()->LoadProgram("gltf_default", "vs_gltf.bin",
                                                "fs_gltf.bin"));
  }

  if (show_gizmos_) {
    // Selection outline first (uses depth from the forward pass)
    for (auto* e : selected_entities) {
      if (e)
        RenderSelectedEntity(e, view_projection, camera);
    }
    Entity selected_entity = EditorUI::Get()->GetSelectedEntity();
    ComponentGizmos::DrawTransformGizmo(selected_entity, viewMatrix,
                                        projMatrix);
  }

  // Skybox
  if (show_skybox_ && ctx.globals.skybox) {
    ctx.globals.skybox->Submit(ViewID::SCENE_FORWARD, viewMatrix, projMatrix,
                               /*useInverseViewProj=*/true);
  }

  // Terrain
  /* if (ctx.globals.terrain) {
    ctx.globals.terrain->Submit(ViewID::SCENE_FORWARD, viewMatrix, projMatrix,
                                camPos);
  }*/

  // TODO: make sure it uses reflection
  // TODO: comment out code after adding water element
  /* if (show_water_) {
    auto* game = static_cast<GameTest*>(Runtime::Game());
    game->RenderWater(viewMatrix, projMatrix, camPos);
    // future: ctx.globals.water->SetReflectionTexture(... from
    // ctx.tex("reflection") ...);
    //         ctx.globals.water->Submit(ViewID::SCENE_FORWARD, viewMatrix,
    //         projMatrix, camPos);
  }*/
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

void SceneViewPipeline::RenderSelectedEntity(EntityContainer* entity,
                                             const glm::mat4& view_projection,
                                             const OrbitCamera& camera) {
  // Render selected entity gizmos if needed
  // TODO: if (gizmos) ComponentGizmos::drawEntityGizmos(*gizmos, *entity);

  // Make sure selected entity is renderable
  if (!entity || !entity->Has<MeshRendererComponent>())
    return;

  // Fetch components
  TransformComponent& transform = entity->Transform();
  MeshRendererComponent& renderer = entity->Get<MeshRendererComponent>();

  // Renderer must be enabled
  if (!renderer.enabled_)
    return;

  // Get camera transform
  // TODO: TransformComponent& cameraTransform = std::get<0>(camera);

  // Render the selected entity and write to stencil
  // TODO: Set BGFX stencil state for outline rendering

  // Forward render entity's base mesh
  // TODO: Set shader uniforms (mvp, model, normal)
  // TODO: should use Shader and Material systems once implemented
  bgfx::setTransform(glm::value_ptr(transform.model_));
  renderer.mesh_->Bind();
  bgfx::setState(BGFX_STATE_DEFAULT);
  bgfx::submit(ViewID::SCENE_FORWARD, program_);

  // Don't render outline if wireframe is enabled
  if (wireframe_)
    return;

  // Render outline of selected entity
  // TODO: Set BGFX stencil state for outline rendering
  // TODO: Enable blending for outline

  // Placeholder transform component for outline
  TransformComponent outline_transform = transform;
  float thickness = 0.038f;
  outline_transform.scale_ += glm::vec3(thickness);

  // Recompute model matrix for outline
  outline_transform.model_ =
      glm::translate(glm::mat4(1.0f), outline_transform.position_) *
      glm::mat4_cast(outline_transform.rotation_) *
      glm::scale(glm::mat4(1.0f), outline_transform.scale_);

  Transform::UpdateMVP(outline_transform, view_projection);

  // Render mesh as outline
  // TODO: should use Shader and Material systems once implemented
  bgfx::setTransform(glm::value_ptr(outline_transform.model_));
  renderer.mesh_->Bind();
  // TODO: Use selection_material_ for outline shader
  bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_BLEND_ALPHA);
  bgfx::submit(ViewID::SCENE_FORWARD, selection_material_);

  // TODO: Reset BGFX blend and stencil state if needed
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
  /*bgfx::ViewId view = ViewID::SCENE_FORWARD;
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
  TransformSystem::Perform(viewProjection);
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

  // Execute frame graph
  Runtime::GetFrameGraph().Render();
}

/*
void SceneViewPipeline::CreatePasses() {}

void SceneViewPipeline::DestroyPasses() {
  scene_view_forward_pass_.Destroy();
  // make frame graph destory passes?
}*/