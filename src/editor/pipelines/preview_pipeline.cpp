#include "preview_pipeline.h"

#include <glm/gtc/type_ptr.hpp>

#include "editor/runtime/runtime.h"
#include "engine/core/logger.h"
#include "engine/core/view_ids.h"
#include "engine/math/transformation.h"
#include "engine/memory/resource_manager.h"
#include "engine/renderer/lighting/light.h"
#include "engine/renderer/model/model.h"

// Static counter to guarantee unique View IDs across all pipeline instances
static uint16_t s_global_preview_view_offset = 0;

PreviewPipeline::PreviewPipeline()
    : preview_program_(BGFX_INVALID_HANDLE),
      outputs_(),
      render_instructions_() {}

void PreviewPipeline::Create() {
  // Load shader
  preview_program_ =
      Runtime::Shader()->LoadProgram("preview", "vs_gltf.bin", "fs_gltf.bin");

  if (!bgfx::isValid(preview_program_)) {
    Logger::getInstance().Log(LogLevel::Error,
                              "PreviewPipeline: Failed to load preview shader");
  }
}

void PreviewPipeline::Destroy() {
  // Destroy all outputs
  for (auto& output : outputs_) {
    if (bgfx::isValid(output.framebuffer)) {
      bgfx::destroy(output.framebuffer);
    }
    // ? Textures created with destroyOnDestroy=true are handled by the FBO
  }
  outputs_.clear();

  Logger::getInstance().Log(LogLevel::Debug, "PreviewPipeline: Destroyed");
}

void PreviewPipeline::Render() {
  if (!bgfx::isValid(preview_program_))
    return;

  for (size_t i = 0; i < render_instructions_.size(); i++) {
    auto& instruction = render_instructions_[i];

    if (!instruction.model)
      continue;

    if (instruction.output_index >= outputs_.size())
      continue;

    PreviewOutput& output = outputs_[instruction.output_index];

    // Recreate framebuffer if resize is pending
    if (output.resize_pending) {
      if (output.width > 0 && output.height > 0) {
        // Destroy old FBO and textures
        if (bgfx::isValid(output.framebuffer))
          bgfx::destroy(output.framebuffer);
        if (bgfx::isValid(output.color_texture))
          bgfx::destroy(output.color_texture);
        if (bgfx::isValid(output.depth_texture))
          bgfx::destroy(output.depth_texture);

        // Recreate
        output.color_texture = bgfx::createTexture2D(
            output.width, output.height, false, 1, bgfx::TextureFormat::BGRA8,
            BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);

        output.depth_texture = bgfx::createTexture2D(
            output.width, output.height, false, 1, bgfx::TextureFormat::D24S8,
            BGFX_TEXTURE_RT | BGFX_TEXTURE_RT_WRITE_ONLY);

        const bgfx::TextureHandle attachments[2] = {output.color_texture,
                                                    output.depth_texture};
        output.framebuffer = bgfx::createFrameBuffer(BX_COUNTOF(attachments),
                                                     attachments, false);
      }
      output.resize_pending = false;
    }

    if (!bgfx::isValid(output.framebuffer))
      continue;

    // Configure preview view
    bgfx::ViewId view_id = output.view_id;

    uint32_t clear_color =
        (uint32_t(instruction.background_color.r * 255) << 24) |
        (uint32_t(instruction.background_color.g * 255) << 16) |
        (uint32_t(instruction.background_color.b * 255) << 8) |
        (uint32_t(instruction.background_color.a * 255));

    bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                       clear_color, 1.0f, 0);
    bgfx::setViewRect(view_id, 0, 0, output.width, output.height);
    bgfx::setViewFrameBuffer(view_id, output.framebuffer);

    // ------ Build view and projection matrices ------
    // Build model matrix
    glm::mat4 model_mtx =
        Transformation::Model(instruction.model_transform.position_,
                              instruction.model_transform.rotation_,
                              instruction.model_transform.scale_);

    glm::mat4 view_mtx =
        Transformation::View(instruction.camera_transform.position_,
                             instruction.camera_transform.rotation_);

    float aspect =
        (output.height > 0) ? float(output.width) / float(output.height) : 1.0f;

    // Explicit radians bypasses Transformation::Projection FOV bugs
    // Near plane set to 0.01f so the camera doesn't slice the model when zoomed

    glm::mat4 proj_mtx =
        Transformation::Projection(45.0f, aspect, 0.3f, 1000.0f);

    bgfx::setViewTransform(view_id, glm::value_ptr(view_mtx),
                           glm::value_ptr(proj_mtx));

    // Submit all meshes of model
    bgfx::setTransform(glm::value_ptr(model_mtx));

    for (int i = 0; i < instruction.model->NLoadedMeshes(); i++) {
      const Mesh* mesh = instruction.model->QueryMesh(i);
      if (!mesh)
        continue;

      // Matte Lighting
      // ----------------------------
      // 1. Set ambient light (Brightened to 0.4 so dark materials are visible)
      Light::SetAmbient(glm::vec3(0.4f));

      // 2. Set primary directional light (Bright white sunlight to pop the
      // model details)
      Light::SetDirectionalLight(
          0, glm::normalize(glm::vec3(-0.5f, -1.0f, -0.5f)),  // Direction
          glm::vec3(0.2f),  // Ambient intensity
          glm::vec3(3.0f),  // Diffuse intensity
          glm::vec3(1.0f)   // Specular intensity
      );

      // 3. using exactly 1 directional light, 0 point, 0 spot
      Light::SetLightCounts(1, 0, 0);

      // 4. Push data to BGFX uniforms for this render pass
      Light::ApplyUniforms();

      mesh->Bind();
      bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_CULL_CW);
      bgfx::submit(view_id, preview_program_);
    }
  }

  render_instructions_.clear();
}

size_t PreviewPipeline::CreateOutput() {
  PreviewOutput output;
  output.width = 250;
  output.height = 250;

  // Create color and depth textures
  output.color_texture = bgfx::createTexture2D(
      output.width, output.height, false, 1, bgfx::TextureFormat::BGRA8,
      BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);

  output.depth_texture = bgfx::createTexture2D(
      output.width, output.height, false, 1, bgfx::TextureFormat::D24S8,
      BGFX_TEXTURE_RT | BGFX_TEXTURE_RT_WRITE_ONLY);

  // Create framebuffer from attachments
  const bgfx::TextureHandle attachments[2] = {output.color_texture,
                                              output.depth_texture};
  output.framebuffer =
      bgfx::createFrameBuffer(BX_COUNTOF(attachments), attachments, false);

  if (!bgfx::isValid(output.framebuffer)) {
    Logger::getInstance().Log(LogLevel::Error,
                              "PreviewPipeline: Failed to create output FBO");
  }

  output.view_id = ViewID::PREVIEW_PASS_BASE + s_global_preview_view_offset++;
  if (output.view_id > ViewID::PREVIEW_PASS_MAX) {
    Logger::getInstance().Log(LogLevel::Error,
                              "PreviewPipeline: Exceeded maximum View IDs!");
  }

  outputs_.push_back(output);
  return outputs_.size() - 1;
}

const PreviewOutput& PreviewPipeline::GetOutput(size_t index) const {
  static const PreviewOutput kEmpty{};

  if (index >= outputs_.size())
    return kEmpty;

  return outputs_[index];
}

bgfx::TextureHandle PreviewPipeline::GetOutputTexture(size_t index) const {
  if (index >= outputs_.size())
    return BGFX_INVALID_HANDLE;

  return outputs_[index].color_texture;
}

void PreviewPipeline::ResizeOutput(size_t index, float width, float height) {
  if (index >= outputs_.size())
    return;

  PreviewOutput& output = outputs_[index];
  uint16_t w = static_cast<uint16_t>(width);
  uint16_t h = static_cast<uint16_t>(height);

  if (w == output.width && h == output.height)
    return;

  output.width = w;
  output.height = h;
  output.resize_pending = true;
}

void PreviewPipeline::AddRenderInstruction(
    const PreviewRenderInstruction instruction) {
  render_instructions_.push_back(instruction);
}
