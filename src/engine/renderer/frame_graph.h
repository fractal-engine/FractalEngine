/**************************************************************************
 * RenderGraph
 * -------------
 * High-level backend-agnostic pass orchestration layer.
 *
 * Models a directed graph of:
 *  - Passes (depth pre-pass, SSAO, forward lighting, post-processing, etc.)
 *  - Attachments (color, depth, intermediate textures)
 *  - Edges (output of pass A feeds as input to pass B)
 *
 * Responsibilities:
 *  - Manages pass sequencing and dependency resolution
 *  - Handles transient texture allocation/reuse
 *  - Rebuilds graph on viewport/resolution changes
 *  - Provides high-level API that abstracts BGFX details
 *
 * Usage:
 *  - Build once at startup via Build(GraphDesc)
 *  - Call Render() each frame
 *  - Rebuild when viewport changes
 *
 * TODOs:
 *  - Pass creation API (addPass, addEdge)
 *  - Dependency tracking between passes
 *  - Resource management?
 *  - Graph-traversal and execution logic?
 *  - Validation of graph
 **************************************************************************/

#ifndef FRAME_GRAPH_H
#define FRAME_GRAPH_H

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class RendererBase;

struct AttachmentDesc {
  std::string name;
  uint16_t width = 0;  // 0 == swap-chain sized
  uint16_t height = 0;
  uint8_t mip = 1;
  bool is_depth = false;
  // TODO: format, flags, clear value
};

struct Pass {
  std::string name;
  std::vector<std::string> reads;
  std::vector<std::string> writes;

  struct Context {
    RendererBase& renderer;
    uint8_t view_id;
    uint16_t width;
    uint16_t height;

    // helpers
    std::function<uint32_t(const std::string&)>
        tex;  // return opaque texture id
    std::function<uint32_t(const std::string&)> fbo;  // return opaque fbo id
  };

  std::function<void(const Context&)> execute;
};

class FrameGraph {
public:
  explicit FrameGraph(RendererBase& renderer) : renderer_(renderer) {}

  // Graph construction
  void AddAttachment(const AttachmentDesc& desc);
  void AddPass(const Pass& pass);

  void Bake();   // topo sort + allocate texture/fbos via renderer_
  void Clear();  // reset to empty graph

  // Per-frame
  void Render();

  // Resize
  void Rebuild(uint16_t width, uint16_t height);

private:
  RendererBase& renderer_;

  struct AttachmentRT {
    AttachmentDesc desc;
    uint32_t texture_id = 0xFFFFFFFF;  // opaque handle from RendererBase
    uint32_t fbo_id = 0xFFFFFFFF;      // use if per-pass is allocated
  };

  std::unordered_map<std::string, AttachmentRT> attachments_;
  std::vector<Pass> passes_;
  std::vector<Pass*> exec_order_;
};

#endif  // FRAME_GRAPH_H