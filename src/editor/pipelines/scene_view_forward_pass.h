#ifndef SCENE_VIEW_FORWARD_PASS_H
#define SCENE_VIEW_FORWARD_PASS_H

#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <glm/glm.hpp>
#include <vector>

#include "editor/runtime/application.h"
#include "engine/core/view_ids.h"
#include "engine/ecs/components/camera.h"
#include "engine/ecs/components/mesh_renderer.h"
#include "engine/ecs/components/transform.h"
#include "engine/ecs/world.h"

class EntityContainer;

class SceneViewForwardPass {
public:
  SceneViewForwardPass();
  ~SceneViewForwardPass();

  void Create(uint32_t msaa_samples);
  void Destroy();

  // Return color texture handle
  bgfx::TextureHandle Render(
      const glm::mat4& view, const glm::mat4& projection,
      const glm::mat4& view_projection, const Camera& camera,
      const std::vector<EntityContainer*>& selected_entities);

  // Settings TODO: values should be defined in implementation file
  bool wireframe_;

private:
  // Framebuffer handles
  bgfx::FrameBufferHandle framebuffer_;
  bgfx::TextureHandle color_texture_;
  bgfx::TextureHandle depth_texture_;

  // TODO: future Viewport forward pass instance should go here
  // const Viewport& viewport;

  // TODO: skybox rendering should go here
  // Skybox* skybox;

  // TODO: gizmos rendering should go here
  // IMGizmo* gizmos;

  // View settings
  uint16_t view_id_;

  // TODO: use unlit outline material once material system exists
  bgfx::ProgramHandle selection_material_ BGFX_INVALID_HANDLE;  // placeholder

  // Cache
  bgfx::ProgramHandle current_program_ BGFX_INVALID_HANDLE;

  static constexpr float default_clear_color_[3] = {0.015f, 0.015f, 0.015f};

  // TODO: Probably better placed somewhere else
  static inline uint32_t PackDefaultClearColor(const float rgb[3]) {
    uint8_t r = static_cast<uint8_t>(rgb[0] * 255.0f);
    uint8_t g = static_cast<uint8_t>(rgb[1] * 255.0f);
    uint8_t b = static_cast<uint8_t>(rgb[2] * 255.0f);
    uint8_t a = 255;
    return (a << 24) | (b << 16) | (g << 8) | r;
  }

  void RenderMeshes(const std::vector<EntityContainer*>& skipped_entities);
  void RenderMesh(const TransformComponent& transform,
                  const MeshRendererComponent& renderer);
  void RenderSelectedEntity(EntityContainer* entity,
                            const glm::mat4& view_projection,
                            const Camera& camera);
};

#endif  // SCENE_VIEW_FORWARD_PASS_H