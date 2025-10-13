#include "scene_view_forward_pass.h"

#include <bx/math.h>
#include <glm/gtc/type_ptr.hpp>

#include "editor/gizmos/component_gizmos.h"
#include "editor/runtime/runtime.h"  // TODO: remove this once pipeline is done
#include "engine/core/engine_globals.h"
#include "engine/core/logger.h"
#include "engine/renderer/graphics_renderer.h"
#include "engine/renderer/model/mesh.h"

SceneViewForwardPass::SceneViewForwardPass()
    : framebuffer_(BGFX_INVALID_HANDLE),
      color_texture_(BGFX_INVALID_HANDLE),
      depth_texture_(BGFX_INVALID_HANDLE),
      view_id_(ViewID::SCENE_FORWARD),
      wireframe_(false),
      selection_material_(BGFX_INVALID_HANDLE) {}

SceneViewForwardPass::~SceneViewForwardPass() {
  Destroy();
}

void SceneViewForwardPass::Create(uint32_t msaa_samples) {
  // Clean up any existing resources
  Destroy();

  // Get renderer from the application
  auto* renderer = static_cast<GraphicsRenderer*>(Runtime::Renderer());

  // Get canvas viewport dimensions from global variables
  uint16_t width = canvasViewportW;
  uint16_t height = canvasViewportH;

  if (width == 0 || height == 0) {
    Logger::getInstance().Log(
        LogLevel::Warning, "SceneViewForwardPass: Invalid viewport dimensions");
    return;
  }

  // Create textures for the framebuffer
  // Color texture with MSAA if samples > 1
  uint64_t flags =
      BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
  bgfx::TextureFormat::Enum format = bgfx::TextureFormat::BGRA8;

  bgfx::TextureHandle color_tex;
  if (msaa_samples > 1) {
    color_tex =
        bgfx::createTexture2D(width, height, false, 1, format, flags, nullptr);
  } else {
    color_tex =
        bgfx::createTexture2D(width, height, false, 1, format, flags, nullptr);
  }

  // Depth texture
  bgfx::TextureHandle depth_tex =
      bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::D24S8,
                            BGFX_TEXTURE_RT | BGFX_TEXTURE_RT_WRITE_ONLY);

  // Create framebuffer with both textures
  bgfx::TextureHandle textures[] = {color_tex, depth_tex};
  framebuffer_ = bgfx::createFrameBuffer(2, textures, true);

  if (!bgfx::isValid(framebuffer_)) {
    Logger::getInstance().Log(
        LogLevel::Error, "SceneViewForwardPass: Failed to create framebuffer");
    return;
  }

  // Store texture handles for later use
  color_texture_ = color_tex;
  depth_texture_ = depth_tex;

  // TODO: Load selection highlighting program (used to highlight selected
  // entities) - we need to create actual shader files
  selection_material_ = Runtime::Shader()->LoadProgram(
      "selection", "vs_selection.bin", "fs_selection.bin");

  // TODO: replace with material implementation
  current_program_ = Runtime::Shader()->LoadProgram(
      "gltf_default", "vs_gltf.bin", "fs_gltf.bin");

  Logger::getInstance().Log(LogLevel::Info,
                            "SceneViewForwardPass: Created successfully");
}

void SceneViewForwardPass::Destroy() {
  // Destroy framebuffer (also destroys attached textures)
  if (bgfx::isValid(framebuffer_)) {
    bgfx::destroy(framebuffer_);
    framebuffer_ = BGFX_INVALID_HANDLE;
  }

  // Reset handles as they are destroyed with framebuffer
  color_texture_ = BGFX_INVALID_HANDLE;
  depth_texture_ = BGFX_INVALID_HANDLE;

  // Destroy selection program
  if (bgfx::isValid(selection_material_)) {
    bgfx::destroy(selection_material_);
    selection_material_ = BGFX_INVALID_HANDLE;
  }

  // Destroy current program
  if (bgfx::isValid(current_program_)) {
    bgfx::destroy(current_program_);
    current_program_ = BGFX_INVALID_HANDLE;
  }
}

bgfx::TextureHandle SceneViewForwardPass::Render(
    const glm::mat4& view, const glm::mat4& projection,
    const glm::mat4& view_projection, const Camera& camera,
    const std::vector<EntityContainer*>& selected_entities,
    Entity selectedEntity) {

  if (!bgfx::isValid(framebuffer_)) {
    Logger::getInstance().Log(
        LogLevel::Warning,
        "SceneViewForwardPass: Attempted to render with invalid framebuffer");
    return BGFX_INVALID_HANDLE;
  }

  // Set view transform for the scene
  bgfx::setViewTransform(view_id_, glm::value_ptr(view),
                         glm::value_ptr(projection));

  // Set framebuffer for this view
  bgfx::setViewFrameBuffer(view_id_, framebuffer_);

  // Set view rect
  bgfx::setViewRect(view_id_, 0, 0, canvasViewportW, canvasViewportH);

  // Choose clear color
  const uint32_t clear_color_ =
      wireframe_ ? 0xffffffff  // white when in wire-frame mode
                 : PackDefaultClearColor(default_clear_color_);

  // Clear the view
  bgfx::setViewClear(view_id_,
                     BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH | BGFX_CLEAR_STENCIL,
                     clear_color_, 1.0f, 0);

  // Set view mode (wireframe if enabled)
  // TODO: wireframe mode should be exposed in future viewport window
  bgfx::setViewMode(view_id_, bgfx::ViewMode::Default);

  // Render each entity
  RenderMeshes(selected_entities);

  // Render selected entities with highlight effect
  for (auto* entity : selected_entities) {
    if (entity) {
      RenderSelectedEntity(entity, view_projection, camera);
    }
  }

  // --- New for Gizmo ---
  // After all meshes are drawn, draw the gizmo on top
  ComponentGizmos::DrawTransformGizmo(selectedEntity, glm::value_ptr(view),
                                      glm::value_ptr(projection));

  // Return the color texture for display
  return color_texture_;
}

void SceneViewForwardPass::RenderMesh(const TransformComponent& transform,
                                      const MeshRendererComponent& renderer) {

  // TODO: Transform components model and mvp must have been calculated
  // beforehand

  // Renderer must be enabled
  if (!renderer.enabled_)
    return;

  // Make sure mesh is available
  if (!renderer.mesh_)
    return;

  // Set shader uniforms
  // TODO: Should use Shader and Material systems once implemented

  // Bind mesh
  renderer.mesh_->Bind();

  // Render mesh
  bgfx::submit(view_id_, current_program_);
}

void SceneViewForwardPass::RenderMeshes(
    const std::vector<EntityContainer*>& skipped_entities) {
  // Track currently bound shader/material
  uint32_t current_shader_id = 0;
  uint32_t current_material_id = 0;
  uint16_t new_bound_shaders = 0;
  uint16_t new_bound_materials = 0;

  for (auto& [entity, transform, renderer] : ECS::Main().GetRenderQueue()) {
    // Skip if entity is in skipped_entities
    bool skip = false;
    for (auto* skipped : skipped_entities) {
      if (skipped && skipped->Handle() == entity) {
        skip = true;
        break;
      }
    }
    if (skip)
      continue;

    // TODO: Use Material/shader system
    /* uint32_t shaderId = renderer.material ? renderer.material->GetShaderId()
     : 0; if (shaderId != current_shader_id) {
       renderer.material->BindShader();
       current_shader_id = shaderId;
       new_bound_shaders++;
     }
     uint32_t materialId = renderer.material ? renderer.material->GetId() : 0;
     if (materialId != current_material_id) {
       renderer.material->Bind();
       current_material_id = materialId;
       new_bound_materials++;
     } */

    RenderMesh(transform, renderer);
  }
}

void SceneViewForwardPass::RenderSelectedEntity(
    EntityContainer* entity, const glm::mat4& view_projection,
    const Camera& camera) {
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
  bgfx::submit(view_id_, current_program_);

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
  // TODO: outline_transform.mvp_ = view_projection * outline_transform.model_;

  // Render mesh as outline
  // TODO: should use Shader and Material systems once implemented
  bgfx::setTransform(glm::value_ptr(outline_transform.model_));
  renderer.mesh_->Bind();
  // TODO: Use selection_material_ for outline shader
  bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_BLEND_ALPHA);
  bgfx::submit(view_id_, selection_material_);

  // TODO: Reset BGFX blend and stencil state if needed
}