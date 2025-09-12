#include "frame_graph.h"

#include <algorithm>
#include "engine/core/logger.h"

// Store the renderer reference
FrameGraph::FrameGraph(GraphicsRenderer& renderer) : renderer_(renderer) {}

// Make sure to destroy resources on shutdown
FrameGraph::~FrameGraph() {
    DestroyPhysicalResources();
}

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
    passes_.push_back(pass);
}

void FrameGraph::Bake() {
    exec_order_.clear();
    // Simple sequential sort for now. TODO: Change to 'reads' and 'writes' for a topological sort.
    for (auto& pass : passes_) {
        exec_order_.push_back(&pass);
    }
    Logger::getInstance().Log(LogLevel::Info, "FrameGraph: Baked with " + std::to_string(passes_.size()) + " passes.");
}

void FrameGraph::Clear() {
    DestroyPhysicalResources();
    attachments_.clear();
    passes_.clear();
    exec_order_.clear();
}

void FrameGraph::DestroyPhysicalResources() {
    for (auto& [name, rt] : attachments_) {
        // The framebuffer handle implicitly manages its texture attachments if created correctly.
        // Destroying the FBO is sufficient.
        renderer_.DestroyFrameBuffer(rt.fbo_handle);
        // If a texture was created standalone, it needs separate destruction.
        renderer_.DestroyTexture(rt.texture_handle);
    }
}

void FrameGraph::Rebuild(uint16_t width, uint16_t height) {
    viewport_width_ = width;
    viewport_height_ = height;

    // 1. Destroy all old BGFX resources before creating new ones
    DestroyPhysicalResources();

    // 2. Create physical textures for all logical attachments
    for (auto& [name, rt] : attachments_) {
        uint16_t tex_width = (rt.desc.width == 0) ? width : rt.desc.width;
        uint16_t tex_height = (rt.desc.height == 0) ? height : rt.desc.height;
        rt.texture_handle = renderer_.CreateTexture2D(tex_width, tex_height, rt.desc.format, rt.desc.flags);
    }

    // 3. Create physical framebuffers for each pass that writes to attachments
    for (Pass* pass : exec_order_) {
        if (pass->writes.empty()) continue;

        std::vector<bgfx::TextureHandle> pass_attachments;
        for (const auto& attachment_name : pass->writes) {
            if (attachments_.count(attachment_name)) {
                pass_attachments.push_back(attachments_.at(attachment_name).texture_handle);
            }
        }
        
        if (!pass_attachments.empty()) {
            // Let BGFX destroy the textures when the FBO is destroyed
            bgfx::FrameBufferHandle fbh = renderer_.CreateFrameBuffer(pass_attachments, true);
            
            // Store this FBO handle in all attachments it writes to
            for (const auto& attachment_name : pass->writes) {
                 attachments_.at(attachment_name).fbo_handle = fbh;
            }
        }
    }
    Logger::getInstance().Log(LogLevel::Debug, "FrameGraph: Rebuilt physical resources for " + std::to_string(width) + "x" + std::to_string(height));
}


void FrameGraph::Render() {
    for (Pass* pass : exec_order_) {
        if (!pass || !pass->execute) continue;

        // The context provides type-safe BGFX handles
        Context ctx{
            .renderer = renderer_,
            .view_id = 0, // TODO: Pass should declare its view ID
            .width = viewport_width_,
            .height = viewport_height_,
            .tex = [this](const std::string& name) -> bgfx::TextureHandle {
                return attachments_.count(name) ? attachments_.at(name).texture_handle : BGFX_INVALID_HANDLE;
            },
            .fbo = [this](const std::string& name) -> bgfx::FrameBufferHandle {
                return attachments_.count(name) ? attachments_.at(name).fbo_handle : BGFX_INVALID_HANDLE;
            }
        };
        pass->execute(ctx);
    }
}