#include "scene_view_pipeline.h"

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "editor/camera/camera_view.h"
#include "editor/editor_ui.h"
#include "editor/gizmos/component_gizmos.h"
#include "editor/runtime/runtime.h"  // TODO: Remove this once pipeline is done
#include "engine/core/engine_globals.h"
#include "engine/core/logger.h"
#include "engine/core/view_ids.h"
#include "engine/ecs/world.h"
#include "engine/math/transformation.h"
#include "engine/renderer/lighting/light.h"
#include "engine/renderer/model/mesh.h"
#include "engine/renderer/shadows/shadow_map.h"
#include "engine/transform/transform.h"

SceneViewPipeline::SceneViewPipeline()
    : wireframe_(false),
      show_skybox_(true),
      msaa_samples_(4),
      view_(glm::mat4(1.0f)),
      projection_(glm::mat4(1.0f)),
      show_terrain_(true),
      show_water_(true),
      show_gizmos_(true),
      god_camera_transform_(),
      god_camera_root_(),
      god_camera_(god_camera_transform_, god_camera_root_),
      render_shadows_(true),
      default_program_(BGFX_INVALID_HANDLE),
      transform_system_(),
      selected_entities_(),
      show_grid_(true),
      selection_program_(BGFX_INVALID_HANDLE),
      grid_mesh_(nullptr) {
  // TODO: Initialize other members e.g. profile, viewport, passes, etc.
  // Check reference project for details
}

void SceneViewPipeline::Create() {
  // TODO: Initialize default profile (exposure, contrast, gamma, etc)?

  // Set up fly camera for scene view
  std::get<1>(god_camera_).fov_ = 90.0f;

  // Load shared shaders
  default_program_ = Runtime::Shader()->LoadProgram(
      "terrain", "vs_vertex_color.bin", "fs_vertex_color.bin");
  selection_program_ = Runtime::Shader()->LoadProgram(
      "selection", "vs_selection.bin", "fs_selection.bin");
  debug_program_ = Runtime::Shader()->LoadProgram("debug", "vs_grid_plane.bin",
                                                  "fs_grid_plane.bin");

  BuildReferenceGrid(glm::mat4(1.0f));
  RegisterNodes();

  Logger::getInstance().Log(LogLevel::Info,
                            "SceneViewPipeline: Frame graph created");
}

void SceneViewPipeline::RegisterNodes() {
  auto& frame_graph = Runtime::GetFrameGraph();

  // Clear existing graph
  frame_graph.Clear();

  // --- ATTACHMENTS ---
  // TODO: GraphicsRenderer should create these
  frame_graph.AddAttachment(
      AttachmentDesc{.name = "shadow_map", .width = 4096, .height = 4096});
  frame_graph.AddAttachment(
      AttachmentDesc{.name = "scene_color", .width = 0, .height = 0});
  frame_graph.AddAttachment(
      AttachmentDesc{.name = "scene_depth", .width = 0, .height = 0});
  frame_graph.AddAttachment(
      AttachmentDesc{.name = "reflection", .width = 0, .height = 0});

  // --- NODES ---
  // Node 0: Shadows
  frame_graph.AddNode(Node{.name = "shadows",
                           .reads = {},
                           .writes = {"shadow_map"},
                           .execute = [this](const Node::Context& ctx) {
                             RenderShadowNode(ctx);
                           }});

  // Node 1: Reflection rendering (for water)
  frame_graph.AddNode(Node{.name = "reflection",
                           .reads = {},
                           .writes = {"reflection"},
                           .execute = [this](const Node::Context& context) {
                             RenderReflectionNode(context);
                           }});

  // Node 2: Forward opaque rendering
  frame_graph.AddNode(Node{.name = "forward_opaque",
                           .reads = {"reflection", "shadow_map"},
                           .writes = {"scene_color", "scene_depth"},
                           .execute = [this](const Node::Context& context) {
                             RenderForwardNode(context);
                           }});

  // Node 3: Selection outline (reads depth from forward pass)
  frame_graph.AddNode(Node{.name = "selection_outline",
                           .reads = {"scene_depth"},
                           .writes = {"scene_color"},
                           .execute = [this](const Node::Context& context) {
                             if (show_gizmos_ && !selected_entities_.empty()) {
                               RenderSelectionOutlineNode(context);
                             }
                           }});

  // Node 4: Gizmos (transform handles, etc.)
  frame_graph.AddNode(Node{.name = "gizmos",
                           .reads = {"scene_depth"},
                           .writes = {"scene_color"},
                           .execute = [this](const Node::Context& context) {
                             if (show_gizmos_) {
                               RenderGizmosNode(context);
                             }
                           }});

  // Bake the graph
  frame_graph.Bake();

  Logger::getInstance().Log(LogLevel::Info,
                            "SceneViewPipeline: Registered nodes");
}

// TODO: implement into a larger standalone render pass (?)
// TODO: we already use SceneCamera& camera in ForwardPass, maybe we should
// delete it here
void SceneViewPipeline::RenderReflectionNode(const Node::Context& context) {
  // Get the GraphicsRenderer
  auto* renderer = static_cast<GraphicsRenderer*>(Runtime::Renderer());
  if (!renderer) {
    Logger::getInstance().Log(
        LogLevel::Error,
        "RenderReflectionPass: Failed to get GraphicsRenderer");
    return;
  }

  // Get reflection framebuffer from GraphicsRenderer
  bgfx::FrameBufferHandle reflection_fb = renderer->GetReflectionFramebuffer();

  // Bind reflection target
  uint16_t fbw = renderer->GetFramebufferWidth();
  uint16_t fbh = renderer->GetFramebufferHeight();

  // Set up BGFX view for reflection
  bgfx::setViewRect(ViewID::REFLECTION_PASS, 0, 0, fbw, fbh);
  bgfx::setViewFrameBuffer(ViewID::REFLECTION_PASS, reflection_fb);
  bgfx::setViewClear(ViewID::REFLECTION_PASS,
                     BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);

  // Get camera matrices
  TransformComponent& camera_transform = std::get<0>(god_camera_);
  CameraComponent& camera_component = std::get<1>(god_camera_);

  float viewMatrix[16];
  float projMatrix[16];
  CameraView::GetViewMatrix(camera_transform, viewMatrix);
  CameraView::GetProjectionMatrix(
      camera_component, projMatrix,
      float(canvasViewportW) / float(canvasViewportH));

  // Build reflected view (Y-plane mirror)
  float reflect[16];
  bx::mtxIdentity(reflect);
  reflect[5] = 1.0f;

  float reflectedView[16];
  bx::mtxMul(reflectedView, viewMatrix, reflect);

  bgfx::setViewTransform(ViewID::REFLECTION_PASS, reflectedView, projMatrix);

  // Render skybox in reflection (rotation-only)
  if (show_skybox_ && context.globals.skybox) {
    float rotation_only[16];
    memcpy(rotation_only, reflectedView, sizeof(rotation_only));
    rotation_only[12] = rotation_only[13] = rotation_only[14] =
        0.0f;  // Remove translation

    context.globals.skybox->Submit(ViewID::REFLECTION_PASS, rotation_only,
                                   projMatrix,
                                   /*useInverseViewProj=*/false);
  }

  // TODO: Render scene geometry for reflection
}

void SceneViewPipeline::RenderForwardNode(const Node::Context& context) {

  // TODO: In this function, we should:
  // Setup view
  // Update transforms
  // Render all scene meshes
  // Draw gizmos

  auto* renderer = static_cast<GraphicsRenderer*>(Runtime::Renderer());
  auto& world = ECS::Main();

  // Get Camera
  TransformComponent& camera_transform = std::get<0>(god_camera_);
  CameraComponent& camera_component = std::get<1>(god_camera_);

  // TODO: revise code here
  // Build view and projection matrices
  view_ = Transformation::View(camera_transform.position_,
                               camera_transform.rotation_);
  projection_ = Transformation::Projection(
      camera_component.fov_, float(canvasViewportW) / float(canvasViewportH),
      camera_component.near_clip_, camera_component.far_clip_);

  float viewMatrix[16];
  float projMatrix[16];
  CameraView::GetViewMatrix(camera_transform, viewMatrix);
  CameraView::GetProjectionMatrix(
      camera_component, projMatrix,
      float(canvasViewportW) / float(canvasViewportH));

  /* Logger::getInstance().Log(
      LogLevel::Debug, "BGFX view[0]: " + std::to_string(viewMatrix[0]) + ", " +
                           std::to_string(viewMatrix[1]) + ", " +
                           std::to_string(viewMatrix[2]));*/

  // Setup BGFX view
  bgfx::setViewTransform(ViewID::SCENE_FORWARD, viewMatrix, projMatrix);
  bgfx::setViewRect(ViewID::SCENE_FORWARD, 0, 0, canvasViewportW,
                    canvasViewportH);

  // Get scene FBO from GraphicsRenderer
  bgfx::setViewFrameBuffer(ViewID::SCENE_FORWARD,
                           renderer->GetSceneFramebuffer());

  bgfx::setViewClear(ViewID::SCENE_FORWARD, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                     wireframe_ ? 0xffffffff : 0x303030ff, 1.0f, 0);

  // Get shadows
  auto& shadow_map = Runtime::MainShadowMap();

  // Build view_projection matrix
  glm::mat4 view = glm::make_mat4(viewMatrix);
  glm::mat4 projection = glm::make_mat4(projMatrix);
  glm::mat4 view_projection = projection * view;

  // Update transforms
  transform_system_.Perform(view_projection);

  // Bulk MVP update
  const auto& render_queue = world.GetRenderQueue();
  for (auto& [entity, transform, renderer_comp] : render_queue) {
    Transform::UpdateMVP(transform, view_projection);
  }

  // Set directional lights uniforms
  auto light_view = world.View<DirectionalLightComponent, TransformComponent>();
  for (auto [e, light, transform] : light_view.each()) {
    if (!light.enabled_)
      continue;

    glm::vec3 direction =
        glm::normalize(Transform::Forward(transform, Space::WORLD));
    Light::SetDirectionalLight(direction, light.color_, light.intensity_);
    break;
  }
  Light::ApplyUniforms();

  // Render all scene meshes
  for (const auto& [entity, transform, renderer_comp] : render_queue) {
    if (!renderer_comp.enabled_ || !renderer_comp.mesh_)
      continue;

    // Skip selected entities (rendered in selection pass)
    bool is_selected = false;
    for (auto* selected : selected_entities_) {
      if (selected && selected->Handle() == entity) {
        is_selected = true;
        break;
      }
    }
    if (is_selected)
      continue;

    // Set shadow uniforms
    glm::mat4 light_space = shadow_map.GetLightSpaceMatrix();
    bgfx::setUniform(shadow_map.GetLightMatrixUniform(),
                     glm::value_ptr(light_space));
    bgfx::setTexture(4, shadow_map.GetSamplerUniform(),
                     shadow_map.GetTexture());

    // Submit mesh
    bgfx::setTransform(glm::value_ptr(transform.model_));
    renderer_comp.mesh_->Bind();

    // Set culling/state
    // ? Create a method for handling render states?
    uint64_t render_state = BGFX_STATE_DEFAULT | BGFX_STATE_CULL_CW;

    // Enable wireframe
    if (wireframe_) {
      render_state |= BGFX_STATE_PT_LINES;
    }

    bgfx::setState(render_state);
    bgfx::submit(ViewID::SCENE_FORWARD, default_program_);
  }

  if (show_gizmos_) {
    // Selection outline first (uses depth from the forward pass)
    for (auto* e : selected_entities_) {
      if (e)
        RenderSelectedEntityOutline(e, view_projection);
    }
    /* Entity selected_entity = EditorUI::Get()->GetSelectedEntity();
    ComponentGizmos::DrawTransformGizmo(selected_entity, viewMatrix,
                                        projMatrix);*/
  }

  // Render skybox
  if (show_skybox_ && context.globals.skybox) {
    context.globals.skybox->Submit(ViewID::SCENE_FORWARD, viewMatrix,
                                   projMatrix,
                                   /*useInverseViewProj=*/true);
  }

  // Terrain
  /* if (context.globals.terrain) {
    context.globals.terrain->Submit(ViewID::SCENE_FORWARD, viewMatrix,
  projMatrix, camPos);
  }*/

  // TODO: make sure it uses reflection
  // TODO: comment out code after adding water element
  /* if (show_water_) {
    auto* game = static_cast<GameTest*>(Runtime::Game());
    game->RenderWater(viewMatrix, projMatrix, camPos);
    // future: context.globals.water->SetReflectionTexture(... from
    // context.tex("reflection") ...);
    //         context.globals.water->Submit(ViewID::SCENE_FORWARD, viewMatrix,
    //         projMatrix, camPos);
  }*/
}

void SceneViewPipeline::RenderSelectionOutlineNode(
    const Node::Context& context) {
  if (wireframe_)
    return;  // No outline in wireframe mode

  // Get Camera
  TransformComponent& camera_transform = std::get<0>(god_camera_);
  CameraComponent& camera_component = std::get<1>(god_camera_);

  float viewMatrix[16];
  float projMatrix[16];
  CameraView::GetViewMatrix(camera_transform, viewMatrix);
  CameraView::GetProjectionMatrix(
      camera_component, projMatrix,
      float(canvasViewportW) / float(canvasViewportH));

  // Build view_projection matrix
  glm::mat4 view = glm::make_mat4(viewMatrix);
  glm::mat4 projection = glm::make_mat4(projMatrix);
  glm::mat4 view_projection = projection * view;

  bgfx::setViewTransform(ViewID::SCENE_FORWARD, viewMatrix, projMatrix);

  // Render each selected entity with outline
  for (auto* entity : selected_entities_) {
    if (!entity || !entity->Has<MeshRendererComponent>())
      continue;

    TransformComponent& transform = entity->Transform();
    MeshRendererComponent& renderer = entity->Get<MeshRendererComponent>();

    if (!renderer.enabled_)
      continue;

    // Render base mesh
    bgfx::setTransform(glm::value_ptr(transform.model_));
    renderer.mesh_->Bind();

    bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_CULL_CW);
    bgfx::submit(ViewID::SCENE_FORWARD, default_program_);

    // Render outline (scaled up)
    RenderSelectedEntityOutline(entity, view_projection);
  }
}

void SceneViewPipeline::RenderShadowNode(const Node::Context& context) {
  auto& world = ECS::Main();
  auto& shadow_map = Runtime::MainShadowMap();

  // Get directional light
  glm::vec3 light_dir(0.0f, -1.0f, 0.0f);

  auto light_view = world.View<DirectionalLightComponent, TransformComponent>();
  for (auto [e, light, transform] : light_view.each()) {
    if (!light.enabled_)
      continue;
    light_dir = glm::normalize(Transform::Forward(transform, Space::WORLD));
    break;
  }

  // Calculate light space matrix
  glm::vec3 light_pos = -light_dir * 100.0f;
  glm::vec3 light_target(0.0f);

  float ortho_size = 50.0f;

  float light_view_arr[16];
  float light_proj_arr[16];

  bx::mtxLookAt(light_view_arr, bx::Vec3(light_pos.x, light_pos.y, light_pos.z),
                bx::Vec3(light_target.x, light_target.y, light_target.z),
                bx::Vec3(0.0f, 1.0f, 0.0f), bx::Handedness::Left);

  bx::mtxOrtho(light_proj_arr, -ortho_size, ortho_size,  // left, right
               -ortho_size, ortho_size,                  // bottom, top
               0.1f, 200.0f,                             // near, far
               0.0f,                                     // offset
               bgfx::getCaps()->homogeneousDepth, bx::Handedness::Left);

  // Build light space matrix
  float light_space_arr[16];
  bx::mtxMul(light_space_arr, light_view_arr, light_proj_arr);

  glm::mat4 light_space = glm::make_mat4(light_space_arr);
  shadow_map.SetLightSpaceMatrix(light_space);

  // Setup BGFX view
  uint16_t resolution = shadow_map.GetResolution();
  bgfx::setViewRect(ViewID::SHADOW_PASS, 0, 0, resolution, resolution);
  bgfx::setViewFrameBuffer(ViewID::SHADOW_PASS, shadow_map.GetFramebuffer());
  bgfx::setViewClear(ViewID::SHADOW_PASS, BGFX_CLEAR_DEPTH, 0, 1.0f, 0);
  bgfx::setViewTransform(ViewID::SHADOW_PASS, light_view_arr, light_proj_arr);

  // Load shadow shader
  static bgfx::ProgramHandle shadow_program = BGFX_INVALID_HANDLE;
  if (!bgfx::isValid(shadow_program)) {
    shadow_program = Runtime::Shader()->LoadProgram("shadow", "vs_shadow.bin",
                                                    "fs_shadow.bin");
  }

  if (!bgfx::isValid(shadow_program)) {
    Logger::getInstance().Log(LogLevel::Error, "Shadow shader failed to load");
    return;
  }

  // Render all meshes to shadow map
  const auto& render_queue = world.GetRenderQueue();
  for (const auto& [entity, transform, renderer_comp] : render_queue) {
    if (!renderer_comp.enabled_ || !renderer_comp.mesh_)
      continue;

    bgfx::setTransform(glm::value_ptr(transform.model_));
    renderer_comp.mesh_->Bind();

    // Shadow pass state
    // write depth, cull front faces
    bgfx::setState(BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS |
                   BGFX_STATE_CULL_CW);
    bgfx::submit(ViewID::SHADOW_PASS, shadow_program);
  }
}

void SceneViewPipeline::RenderSelectedEntityOutline(
    EntityContainer* entity, const glm::mat4& view_projection) {
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

  // Placeholder transform component for outline
  TransformComponent outline_transform = transform;
  float thickness = 0.038f;
  outline_transform.scale_ += glm::vec3(thickness);

  // Recompute model matrix for outline
  outline_transform.model_ =
      glm::translate(glm::mat4(1.0f), outline_transform.position_) *
      glm::mat4_cast(outline_transform.rotation_) *
      glm::scale(glm::mat4(1.0f), outline_transform.scale_);

  // Get camera transform
  TransformComponent& cameraTransform = std::get<0>(god_camera_);

  // Render the selected entity and write to stencil
  // TODO: Set BGFX stencil state for outline rendering

  // Forward render entity's base mesh
  // TODO: Set shader uniforms (mvp, model, normal)
  // TODO: should use Shader and Material systems once implemented
  bgfx::setTransform(glm::value_ptr(transform.model_));
  renderer.mesh_->Bind();
  bgfx::setState(BGFX_STATE_DEFAULT);
  bgfx::submit(ViewID::SCENE_FORWARD, default_program_);

  // Render outline of selected entity
  // TODO: Set BGFX stencil state for outline rendering
  // TODO: Enable blending for outline

  // Transform::UpdateMVP(outline_transform, view_projection);

  // Render mesh as outline
  // TODO: should use Shader and Material systems once implemented?
  // TODO: Use stencil test to only draw outline edges
  bgfx::setTransform(glm::value_ptr(outline_transform.model_));
  renderer.mesh_->Bind();
  // TODO: Use selection_material_ for outline shader
  bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_BLEND_ALPHA |
                 BGFX_STATE_CULL_CW);
  bgfx::submit(ViewID::SCENE_FORWARD, selection_program_);

  // TODO: Reset BGFX blend and stencil state if needed
}

void SceneViewPipeline::BuildReferenceGrid(const glm::mat4& view_projection) {
  float size = 10000.0f;

  grid_data.positions = {-size, 0.0f, -size, size,  0.0f, -size,
                         size,  0.0f, size,  -size, 0.0f, size};
  grid_data.normals = {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                       0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f};
  grid_data.indices = {0, 1, 2, 0, 2, 3};

  grid_mesh_ = new Mesh(grid_data);
}

// TODO: should be used for IMGizmo rendering
void SceneViewPipeline::RenderGizmosNode(const Node::Context& context) {
  if (!show_gizmos_)
    return;

  IMGizmo& gizmos = Runtime::SceneGizmos();

  // Get Camera
  TransformComponent& camera_transform = std::get<0>(god_camera_);
  CameraComponent& camera_component = std::get<1>(god_camera_);

  float viewMatrix[16];
  float projMatrix[16];
  CameraView::GetViewMatrix(camera_transform, viewMatrix);
  CameraView::GetProjectionMatrix(
      camera_component, projMatrix,
      float(canvasViewportW) / float(canvasViewportH));

  // Setup BGFX view for gizmo rendering
  bgfx::setViewTransform(ViewID::SCENE_FORWARD, viewMatrix, projMatrix);
  bgfx::setViewRect(ViewID::SCENE_FORWARD, 0, 0, canvasViewportW,
                    canvasViewportH);

  auto* renderer = static_cast<GraphicsRenderer*>(Runtime::Renderer());
  bgfx::setViewFrameBuffer(ViewID::SCENE_FORWARD,
                           renderer->GetSceneFramebuffer());

  // Build view-projection for gizmo rendering
  glm::mat4 view = glm::make_mat4(viewMatrix);
  glm::mat4 projection = glm::make_mat4(projMatrix);
  glm::mat4 view_projection = projection * view;

  // TODO: Draw transform gizmo for selected entity
  /* Entity selected_entity = EditorUI::Get()->GetSelectedEntity();
  if (selected_entity != entt::null) {
    ComponentGizmos::DrawTransformGizmo(selected_entity, viewMatrix,
                                        projMatrix);
  }*/

  // Show grid
  if (show_grid_ && bgfx::isValid(debug_program_) && grid_mesh_) {
    // Snap grid to camera XZ position (infinite grid effect)
    glm::vec3 cam_pos = camera_transform.position_;
    glm::vec3 grid_origin =
        glm::vec3(floor(cam_pos.x / 10.0f) * 10.0f,  // Snap to 10-unit grid
                  0.0f, floor(cam_pos.z / 10.0f) * 10.0f);

    // Set grid origin transform
    glm::mat4 grid_transform = glm::translate(glm::mat4(1.0f), grid_origin);
    bgfx::setTransform(glm::value_ptr(grid_transform));

    // Pass camera position to shader for fog/fade
    glm::vec4 cam_pos_uniform(cam_pos.x, cam_pos.y, cam_pos.z, 0.0f);
    bgfx::setUniform(renderer->GetViewPosUniform(), &cam_pos_uniform);

    grid_mesh_->Bind();
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_BLEND_ALPHA);
    bgfx::submit(ViewID::SCENE_FORWARD, debug_program_);
  }

  gizmos.RenderAll(view_projection);
}

// PLACEHOLDER: Remove this once pipeline is done
void SceneViewPipeline::Render() {
  // TODO: add missing inits and calls noted in RealRender?

  // Start gizmos frame
  IMGizmo& gizmos = Runtime::SceneGizmos();
  gizmos.NewFrame(ViewID::SCENE_FORWARD);
  // ComponentGizmos::DrawSceneViewIcons(gizmos, CameraTransform);

  // Execute frame graph
  Runtime::GetFrameGraph().Render();
}

void SceneViewPipeline::Destroy() {
  if (bgfx::isValid(default_program_)) {
    bgfx::destroy(default_program_);
    default_program_ = BGFX_INVALID_HANDLE;
  }

  if (bgfx::isValid(selection_program_)) {
    bgfx::destroy(selection_program_);
    selection_program_ = BGFX_INVALID_HANDLE;
  }

  if (bgfx::isValid(debug_program_)) {
    bgfx::destroy(debug_program_);
    debug_program_ = BGFX_INVALID_HANDLE;
  }

  delete grid_mesh_;
  grid_mesh_ = nullptr;

  Logger::getInstance().Log(LogLevel::Debug,
                            "[Render] SceneViewPipeline: Destroyed");
}

// TODO: rename to Render() once placeholder is removed
/* void SceneViewPipeline::RealRender() {
// debug
Logger::getInstance().Log(LogLevel::Debug,
                          "[Render] SceneViewPipeline::Render called");

// Set default view and matrices
bgfx::ViewId view = ViewID::SCENE_FORWARD;
glm::mat4 view_mtx = glm::mat4(1.0f);
glm::mat4 proj_mtx = glm::mat4(1.0f);

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
auto& world = ECS::Main();
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
}*/

// POST PROCESSING PASS
// TODO: Render post-processing effects here
// use forward pass output as input

// TODO: Stop profiler ("scene_view")
// }

/* void SceneViewPipeline::CreatePasses() {}

void SceneViewPipeline::DestroyPasses() {
  scene_view_forward_pass_.Destroy();
  // make frame graph destory passes?
}*/

Camera& SceneViewPipeline::GetGodCamera() {
  return god_camera_;
}

const glm::mat4& SceneViewPipeline::GetView() const {
  return view_;
}

const glm::mat4& SceneViewPipeline::GetProjection() const {
  return projection_;
}