#include "scene_view_pipeline.h"

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "engine/core/engine_globals.h"
#include "engine/ecs/world.h"  // TODO: remove later
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

void SceneViewPipeline::Render(bgfx::ViewId view, const float* view_mtx,
                               const float* proj_mtx) {

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
  world.UpdateTransforms();
  const auto& render_queue = world.GetRenderQueue();

  bgfx::setViewTransform(view, view_mtx, proj_mtx);

  for (const auto& item : render_queue) {

    const auto& transform = std::get<1>(item);
    const auto& renderer = std::get<2>(item);

    if (!renderer.enabled_ || !renderer.mesh_)
      continue;

    bgfx::setTransform(glm::value_ptr(transform.model_));
    renderer.mesh_->Bind();
    bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_CULL_CW);
    bgfx::submit(view, program_);
  }

  // POST PROCESSING PASS
  // TODO: Render post-processing effects here
  // use forward pass output as input

  // TODO: Stop profiler ("scene_view")
}
