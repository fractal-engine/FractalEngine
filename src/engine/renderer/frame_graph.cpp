#include "frame_graph.h"

#include <algorithm>
#include "engine/core/logger.h"

void FrameGraph::AddAttachment(const AttachmentDesc& desc) {
  // Add attachment to graph
  if (attachments_.find(desc.name) != attachments_.end()) {
    Logger::getInstance().Log(LogLevel::Warning,
                              "FrameGraph: Attachment '" + desc.name +
                                  "' already exists, overwriting");
  }

  AttachmentRT rt;
  rt.desc = desc;
  attachments_[desc.name] = rt;
}

void FrameGraph::AddPass(const Pass& pass) {
  // Add pass to graph
  passes_.push_back(pass);
}

void FrameGraph::Bake() {
  // Reset execution order
  exec_order_.clear();

  // TODO: Basic sequential topo sort, enhance later
  for (auto& pass : passes_) {
    exec_order_.push_back(&pass);
  }

  Logger::getInstance().Log(
      LogLevel::Info,
      "FrameGraph: Baked with " + std::to_string(attachments_.size()) +
          " attachments and " + std::to_string(passes_.size()) + " passes");
}

void FrameGraph::Clear() {
  // Reset graph
  attachments_.clear();
  passes_.clear();
  exec_order_.clear();
}

void FrameGraph::Render() {
  // Execute passes in order
  for (Pass* pass : exec_order_) {
    if (pass && pass->execute) {
      // Create context for specified pass
      Pass::Context ctx{
          .renderer = renderer_,
          .view_id = 0,  // Will be set properly in future
          .width = 0,    // Will be set properly in future
          .height = 0,   // Will be set properly in future

          // Lambda to get texture ID
          .tex = [this](const std::string& name) -> uint32_t {
            auto it = attachments_.find(name);
            return (it != attachments_.end()) ? it->second.texture_id
                                              : 0xFFFFFFFF;
          },

          // Lambda to get FBO ID
          .fbo = [this](const std::string& name) -> uint32_t {
            auto it = attachments_.find(name);
            return (it != attachments_.end()) ? it->second.fbo_id : 0xFFFFFFFF;
          }};

      // Execute pass
      ctx.globals = globals_;
      pass->execute(ctx);
    }
  }
}

void FrameGraph::Rebuild(uint16_t width, uint16_t height) {
  // Update dimensions of attachments
  for (auto& [name, rt] : attachments_) {
    if (rt.desc.width == 0)
      rt.desc.width = width;
    if (rt.desc.height == 0)
      rt.desc.height = height;
  }

  // Re-bake graph
  Bake();

  Logger::getInstance().Log(LogLevel::Debug,
                            "FrameGraph: Rebuilt with dimensions " +
                                std::to_string(width) + "x" +
                                std::to_string(height));
}