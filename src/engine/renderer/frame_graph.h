/**************************************************************************
 * FrameGraph
 * ----------
 * Backend-agnostic node orchestration layer owned by Runtime.
 *
 * Models:
 *  - Passes through nodes (depth pre-pass, SSAO, forward lighting, post-FX, etc.)
 *  - Attachments (color/depth/intermediate) as logical IDs only
 *  - Implicit edges via node.reads / node.writes
 *
 * Responsibilities:
 *  - Node registration (AddNode) and attachment registration (AddAttachment)
 *  - Simple sequencing (Bake -> current: insertion order)
 *  - Per-frame execution (Render) invoking node lambdas
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

#include "engine/renderer/skybox/skybox.h"

class RendererBase;

struct AttachmentDesc {
  std::string name;
  uint16_t width = 0;  // 0 == swap-chain sized
  uint16_t height = 0;
  uint8_t mip = 1;
  bool is_depth = false;
  // TODO: format, flags, clear value
};

struct GlobalResources {
  Skybox* skybox = nullptr;
  // IMGizmo* scene_gizmos = nullptr;
  // ShadowMap* shadow_map = nullptr;
  // Terrain* terrain = nullptr;
  // Water* water = nullptr;
};

struct Node {
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

    GlobalResources globals;
  };

  std::function<void(const Context&)> execute;
};

class FrameGraph {
public:
  explicit FrameGraph(RendererBase& renderer) : renderer_(renderer) {}

  void SetGlobalResources(const GlobalResources& resources) {
    globals_ = resources;
  }

  // Graph construction
  void AddAttachment(const AttachmentDesc& desc);
  void AddNode(const Node& node);

  void Bake();   // topo sort + allocate texture/fbos via renderer_
  void Clear();  // reset to empty graph

  // Per-frame
  void Render();

  // Resize
  void Rebuild(uint16_t width, uint16_t height);

private:
  RendererBase& renderer_;
  GlobalResources globals_;

  struct AttachmentRT {
    AttachmentDesc desc;
    uint32_t texture_id = 0xFFFFFFFF;  // opaque handle from RendererBase
    uint32_t fbo_id = 0xFFFFFFFF;      // use if per-pass is allocated
  };

  std::unordered_map<std::string, AttachmentRT> attachments_;
  std::vector<Node> nodes_;
  std::vector<Node*> exec_order_;
};

#endif  // FRAME_GRAPH_H