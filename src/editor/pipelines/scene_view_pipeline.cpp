#include "scene_view_pipeline.h"
#include "editor/runtime/runtime.h"
#include "editor/editor_ui.h"
#include "editor/gizmos/component_gizmos.h"
#include "engine/core/logger.h"
#include "engine/core/view_ids.h"
#include "engine/ecs/world.h"
#include "engine/renderer/model/mesh.h"
#include <glm/gtc/type_ptr.hpp>

SceneViewPipeline::SceneViewPipeline() {}
SceneViewPipeline::~SceneViewPipeline() {}

void SceneViewPipeline::RegisterPasses(FrameGraph& frame_graph) {
    // 1. Define the logical attachments this pipeline produces.
    // The FrameGraph will turn these into physical BGFX textures.
    AttachmentDesc color_desc;
    color_desc.name = "scene_color";
    color_desc.format = bgfx::TextureFormat::BGRA8;
    color_desc.flags = BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;

    AttachmentDesc depth_desc;
    depth_desc.name = "scene_depth";
    depth_desc.format = bgfx::TextureFormat::D24S8;
    depth_desc.flags = BGFX_TEXTURE_RT | BGFX_TEXTURE_RT_WRITE_ONLY;

    frame_graph.AddAttachment(color_desc);
    frame_graph.AddAttachment(depth_desc);

    // 2. Define the render pass.
    const uint16_t scene_view_id = Runtime::Renderer()->ReserveViewId();

    FrameGraph::Pass scene_pass;
    scene_pass.name = "Scene_ForwardAndGizmos";
    // This pass writes to both attachments, so they will be bound together in an FBO.
    scene_pass.writes = {"scene_color", "scene_depth"};
    
    scene_pass.execute = [this, scene_view_id](const FrameGraph::Context& context) {
        // The context provides everything we need.
        this->RenderSceneAndGizmos(context, scene_view_id);
    };

    frame_graph.AddPass(scene_pass);

    // 3. Pre-load shader programs.
    gltf_program_ = Runtime::Shader()->LoadProgram("gltf_default", "vs_gltf.bin", "fs_gltf.bin");
    Logger::getInstance().Log(LogLevel::Info, "SceneViewPipeline registered its passes.");
}

void SceneViewPipeline::RenderSceneAndGizmos(const FrameGraph::Context& context, uint16_t view_id) {
    // Get the physical framebuffer for this pass from the context by naming one of its attachments.
    bgfx::FrameBufferHandle fbh = context.fbo("scene_color");
    bgfx::setViewFrameBuffer(view_id, fbh);
    bgfx::setViewRect(view_id, 0, 0, context.width, context.height);
    bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);

    // Get live camera matrices from the editor.
    OrbitCamera& camera = EditorUI::Get()->GetCamera();
    float viewMatrix[16], projMatrix[16];
    camera.getViewMatrix(viewMatrix);
    camera.getProjectionMatrix(projMatrix, float(context.width) / float(context.height));
    bgfx::setViewTransform(view_id, viewMatrix, projMatrix);
    
    // --- Render Scene Meshes ---
    auto& world = ECS::Main();
    world.UpdateTransforms();

    for (const auto& [entity, transform, renderer] : world.GetRenderQueue()) {
        if (!renderer.enabled_ || !renderer.mesh_) continue;
        bgfx::setTransform(glm::value_ptr(transform.model_));
        renderer.mesh_->Bind();
        bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_CULL_CW);
        bgfx::submit(view_id, gltf_program_);
    }

    // --- Render Gizmos on top ---
    Entity selectedEntity = EditorUI::Get()->GetSelectedEntity();
    ComponentGizmos::DrawTransformGizmo(selectedEntity, viewMatrix, projMatrix);
}

void SceneViewPipeline::Destroy() {
    if (bgfx::isValid(gltf_program_)) {
        bgfx::destroy(gltf_program_);
        gltf_program_ = BGFX_INVALID_HANDLE;
    }
}