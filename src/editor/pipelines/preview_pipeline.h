#ifndef PREVIEW_PIPELINE_H
#define PREVIEW_PIPELINE_H

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <vector>

#include "engine/ecs/ecs_collection.h"

class Model;

struct PreviewOutput {
  bgfx::TextureHandle color_texture = BGFX_INVALID_HANDLE;
  bgfx::TextureHandle depth_texture = BGFX_INVALID_HANDLE;
  bgfx::FrameBufferHandle framebuffer = BGFX_INVALID_HANDLE;

  uint16_t width = 0;
  uint16_t height = 0;
  bool resize_pending = false;
};

struct PreviewRenderInstruction {
  size_t output_index = 0;
  glm::vec4 background_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
  Model* model = nullptr;
  // ! add lighting?
  TransformComponent model_transform;
  TransformComponent camera_transform;
};

class PreviewPipeline {

public:
  PreviewPipeline();

  void Create();
  void Destroy();
  void Render();

  // Create new preview output and return its index
  size_t CreateOutput();

  // Return output by given index
  const PreviewOutput& GetOutput(size_t index) const;

  // Get texture handle for ImGui rendering
  bgfx::TextureHandle GetOutputTexture(size_t index) const;

  // Resize output by given index
  void ResizeOutput(size_t index, float width, float height);

  // Add render instruction to the render queue
  void AddRenderInstruction(const PreviewRenderInstruction instruction);

private:
  // Program handle for rendering previews
  bgfx::ProgramHandle preview_program_;

  // All outputs
  std::vector<PreviewOutput> outputs_;

  // Queued render instructions
  std::vector<PreviewRenderInstruction> render_instructions_;
};

#endif  // PREVIEW_PIPELINE_H