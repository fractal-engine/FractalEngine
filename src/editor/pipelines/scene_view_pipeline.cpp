#include "scene_view_pipeline.h"

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "editor/gizmos/scene_view_gizmo.h" 
#include "editor/editor_ui.h"
#include "engine/ecs/world.h"
#include "editor/runtime/runtime.h"  // TODO: Remove this once pipeline is done
#include "engine/core/engine_globals.h"
#include "engine/core/logger.h"
#include "engine/core/view_ids.h"
#include "engine/renderer/model/mesh.h"

SceneViewPipeline::SceneViewPipeline()
    : wireframe_(false), msaa_samples_(4), frame_initialized_(false) {
  // TODO: Initialize other members e.g. profile, camera, viewport, passes, etc.
  // Check reference project for details
}

void SceneViewPipeline::Create() {
  // TODO: Initialize default profile (exposure, contrast, gamma, etc)

  // TODO: set up fly camera for scene view
  // TODO: create passes
  // TODO: set future game view for gizmos
  // Use GameViewPipeline for the gizmos

  // TODO: remove this once the above is implemented
  // Load the shader program ONCE when the pipeline is created.
  m_gltf_program = Runtime::Shader()->LoadProgram("gltf_default", "vs_gltf.bin",
                                                  "fs_gltf.bin");

  if (!bgfx::isValid(m_gltf_program)) {
    Logger::getInstance().Log(LogLevel::Error,
                              "Failed to load gltf_default shader program!");
  }
}

void SceneViewPipeline::Destroy() {

  // TODO: simply call destroyPasses();

  if (bgfx::isValid(m_gltf_program)) {
    bgfx::destroy(m_gltf_program);
    m_gltf_program = BGFX_INVALID_HANDLE;
  }
}

// TODO: rename to Render() once placeholder is removed
void SceneViewPipeline::RealRender() {
  // debug
  Logger::getInstance().Log(LogLevel::Debug,
                            "[Render] SceneViewPipeline::Render called");

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
  Logger::getInstance().Log(LogLevel::Debug,
                            "[Render] SceneViewPipeline::Render called");
  auto& world = ECS::Main();

  // 1. Get the LIVE editor OrbitCamera from the EditorUI singleton.
  // This is the camera controlled by the user in the editor.
  OrbitCamera& camera = EditorUI::Get()->GetCamera();

  // 2. Calculate the LIVE view and projection matrices FROM THE ORBIT CAMERA.
  // Note: OrbitCamera uses float arrays, not glm::mat4, so we adapt.
  float viewMatrix[16];
  float projMatrix[16];
  camera.getViewMatrix(viewMatrix);
  camera.getProjectionMatrix(projMatrix,
                             float(canvasViewportW) / float(canvasViewportH));

  // 3. Set up the BGFX view using the matrices from the OrbitCamera.
  bgfx::setViewTransform(ViewID::SCENE_MESH, viewMatrix, projMatrix);
  bgfx::setViewRect(ViewID::SCENE_MESH, 0, 0, canvasViewportW, canvasViewportH);
  bgfx::setViewClear(ViewID::SCENE_MESH, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                     0x303030ff, 1.0f, 0);

  // 4. Update transforms and get the render queue.
  world.UpdateTransforms();
  const auto& render_queue = world.GetRenderQueue();

  // 5. Render all the meshes in the scene.
  for (const auto& [entity, transform, renderer] : render_queue) {
    if (!renderer.enabled_ || !renderer.mesh_)
      continue;

    bgfx::setTransform(glm::value_ptr(transform.model_));
    renderer.mesh_->Bind();
    bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_CULL_CW);
    // Use the cheap, pre-loaded program handle. Do NOT load from disk here.
    if (bgfx::isValid(m_gltf_program)) {
      bgfx::submit(ViewID::SCENE_MESH, m_gltf_program);
    }
  }

  // 6. Get the selected entity from the UI singleton.
  Entity selectedEntity = EditorUI::Get()->GetSelectedEntity();

  // 7. Draw the gizmo ONCE, after all meshes have been rendered.
  // We pass the matrices we got from the OrbitCamera.
  m_scene_view_gizmo.OnRender(EditorUI::Get()->GetCamera(), selectedEntity);
}
