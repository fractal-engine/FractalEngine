/**************************************************************************
 * FrameGraph
 * ----------
 * Backend-agnostic pass orchestration layer owned by Runtime.
 *
 * Models:
 *  - Passes (depth pre-pass, SSAO, forward lighting, post-FX, etc.)
 *  - Attachments (color/depth/intermediate) as logical IDs only
 *  - Implicit edges via pass.reads / pass.writes
 *
 * Responsibilities:
 *  - Pass registration (AddPass) and attachment registration (AddAttachment)
 *  - Simple sequencing (Bake -> current: insertion order)
 *  - Per-frame execution (Render) invoking pass lambdas
 *  - Resize hook (Rebuild) updating logical sizes and re-baking
 *
 * TODO:
 *  - Topological sort from reads/writes (real dependency resolution)
 *  - Attachment validation (unwritten reads, multiple writers, cycles)
 *  - Transient resource allocation/reuse via RendererBase (populate texture/fbo
 *ids)
 *  - Attachment metadata: format, clear values, sample count, flags
 *  - Populate Context.view_id / width / height consistently (policy)
 *  - Build(GraphDesc) as a single-shot construction API? 
 **************************************************************************/

#ifndef FRAME_GRAPH_H
#define FRAME_GRAPH_H

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

// --- Change 1: Include BGFX types and forward-declare our concrete renderer ---
// We need the concrete BGFX types to manage physical resources.
#include <bgfx/bgfx.h> 
class RendererBase; 

// --- Change 2: Enhance AttachmentDesc with creation info ---
struct AttachmentDesc {
  std::string name;
  uint16_t width = 0;  // 0 == swap-chain sized
  uint16_t height = 0;
  
  // ADDED: Provide the necessary info for texture creation
  bgfx::TextureFormat::Enum format = bgfx::TextureFormat::BGRA8;
  uint64_t flags = BGFX_TEXTURE_RT; // e.g., BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP

  // REMOVED: is_depth is replaced by the more specific 'format'
};

struct Pass {
  std::string name;
  std::vector<std::string> reads;
  std::vector<std::string> writes;

  // --- Change 3: Update Context to use real, type-safe BGFX handles ---
  struct Context {
    GraphicsRenderer& renderer; // Use the concrete renderer for BGFX calls
    uint8_t view_id;
    uint16_t width;
    uint16_t height;

    // The lambdas now return actual BGFX handles, not opaque integers.
    std::function<bgfx::TextureHandle(const std::string&)> tex;
    std::function<bgfx::FrameBufferHandle(const std::string&)> fbo;
  };

  std::function<void(const Context&)> execute;
};

class FrameGraph {
public:
  // --- Change 4: Depend on the concrete GraphicsRenderer to do BGFX work ---
  explicit FrameGraph(GraphicsRenderer& renderer) : renderer_(renderer) {}
  

  // Graph construction
  void AddAttachment(const AttachmentDesc& desc);
  void AddPass(const Pass& pass);

  void Bake();
  void Clear();

  // Per-frame
  void Render();

  // Resize
  void Rebuild(uint16_t width, uint16_t height);

   // A public getter for the UI to get the final texture handle
  bgfx::TextureHandle GetAttachmentTexture(const std::string& name) {
      if (attachments_.count(name)) {
          return attachments_.at(name).texture_handle;
      }
      return BGFX_INVALID_HANDLE;
  }

private:
  // We need the concrete facade to call CreateTexture2D, etc.
  GraphicsRenderer& renderer_;

  // --- Change 5: AttachmentRT holds physical BGFX handles ---
  struct AttachmentRT {
    AttachmentDesc desc;
    bgfx::TextureHandle texture_handle = BGFX_INVALID_HANDLE;
    bgfx::FrameBufferHandle fbo_handle = BGFX_INVALID_HANDLE;
  };

  std::unordered_map<std::string, AttachmentRT> attachments_;
  std::vector<Pass> passes_;
  std::vector<Pass*> exec_order_;
};

#endif  // FRAME_GRAPH_H