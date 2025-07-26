#include "scene_view_pipeline.h"

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

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
  if (bgfx::isValid(program_))
    return;
}

void SceneViewPipeline::Destroy() {

  // TODO: simply call destroyPasses();

  // TODO: remove this once the above is implemented
  if (bgfx::isValid(program_))
    bgfx::destroy(program_), program_ = BGFX_INVALID_HANDLE;
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

  // Basic setup
  glm::mat4 view =
      glm::lookAt(glm::vec3(0, 2, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
  glm::mat4 proj = glm::perspective(
      glm::radians(60.0f), float(canvasViewportW) / float(canvasViewportH),
      0.1f, 100.0f);

  bgfx::setViewTransform(ViewID::SCENE_MESH, glm::value_ptr(view),
                         glm::value_ptr(proj));
  bgfx::setViewRect(ViewID::SCENE_MESH, 0, 0, canvasViewportW, canvasViewportH);
  bgfx::setViewClear(ViewID::SCENE_MESH, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                     0x303030ff, 1.0f, 0);

  // Iterate ECS render queue and submit meshes
  auto& world = ECS::Main();
  world.UpdateTransforms();
  const auto& render_queue = world.GetRenderQueue();
  Logger::getInstance().Log(
      LogLevel::Debug,
      "[Render] Render queue size: " + std::to_string(render_queue.size()));

  for (const auto& [entity, transform, renderer] : render_queue) {
    Logger::getInstance().Log(
        LogLevel::Debug,
        "[Render] Entity: " + std::to_string((int)entity) +
            " enabled: " + std::to_string(renderer.enabled_) + " mesh: " +
            std::to_string(reinterpret_cast<uintptr_t>(renderer.mesh_)));
    if (!renderer.enabled_ || !renderer.mesh_)
      continue;

    bgfx::setTransform(glm::value_ptr(transform.model_));
    renderer.mesh_->Bind();
    bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_CULL_CW);
    // Use your default shader (replace with your actual handle)
    bgfx::submit(ViewID::SCENE_MESH,
                 Runtime::Shader()->LoadProgram("gltf_default", "vs_gltf.bin",
                                                "fs_gltf.bin"));
  }
}