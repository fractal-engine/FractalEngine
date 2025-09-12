#include "graphics_renderer.h"
#include "engine/core/logger.h"
#include "platform/platform_utils.h"
#include "engine/core/view_ids.h" // For UI_BACKGROUND constant

// Constructor is now very simple
GraphicsRenderer::GraphicsRenderer() {
    Logger::getInstance().Log(LogLevel::Info, "GraphicsRenderer created.");
}

GraphicsRenderer::~GraphicsRenderer() {}

bool GraphicsRenderer::Initialize(SDL_Window* window) {
    window_ = window;
    if (!window_) {
        Logger::getInstance().Log(LogLevel::Error, "GraphicsRenderer received an invalid window handle.");
        return false;
    }

    bgfx::Init init;
    platform::SetupBGFXPlatformData(init, window_);
    init.type = bgfx::RendererType::Count; // Auto-select
    init.resolution.width = WindowManager::GetWidth();
    init.resolution.height = WindowManager::GetHeight();
    init.resolution.reset = BGFX_RESET_VSYNC;

    if (!bgfx::init(init)) {
        Logger::getInstance().Log(LogLevel::Error, "Failed to initialize BGFX!");
        return false;
    }

    // This is the ONLY view the renderer itself manages. It's for the final clear.
    bgfx::setViewClear(ViewID::UI_BACKGROUND, BGFX_CLEAR_COLOR, 0x1e1e1eff, 1.0f, 0);
    bgfx::setViewRect(ViewID::UI_BACKGROUND, 0, 0, WindowManager::GetWidth(), WindowManager::GetHeight());

    // Register our internal resize handler
    WindowManager::RegisterResizeCallback([this](int width, int height) { OnResize(width, height); });

    auto backend = bgfx::getRendererType();
    Logger::getInstance().Log(LogLevel::Info, "BGFX initialized successfully! Backend: " + std::string(bgfx::getRendererName(backend)));
    return true;
}

void GraphicsRenderer::OnResize(uint16_t w, uint16_t h) {
    bgfx::reset(w, h, BGFX_RESET_VSYNC);
    // The background view needs to be updated to the new backbuffer size
    bgfx::setViewRect(ViewID::UI_BACKGROUND, 0, 0, w, h);
}

void GraphicsRenderer::BeginFrame() {
    // This is a good place to ensure the background view is touched,
    // guaranteeing the screen is cleared every frame.
    bgfx::touch(ViewID::UI_BACKGROUND);
}

void GraphicsRenderer::EndFrame() {
    // Present the backbuffer
    bgfx::frame();
}

void GraphicsRenderer::Shutdown() {
    Logger::getInstance().Log(LogLevel::Info, "Shutting down GraphicsRenderer");
    bgfx::shutdown();
    Logger::getInstance().Log(LogLevel::Info, "GraphicsRenderer shutdown complete");
}

uint16_t GraphicsRenderer::ReserveViewId() {
    // Simple atomic increment would be better for multi-threading, but this is fine for now.
    return view_id_counter_++;
}

// --- Generic Resource Wrappers ---

bgfx::FrameBufferHandle GraphicsRenderer::CreateFrameBuffer(const std::vector<bgfx::TextureHandle>& attachments, bool destroyTextures) {
    return bgfx::createFrameBuffer(static_cast<uint8_t>(attachments.size()), attachments.data(), destroyTextures);
}

void GraphicsRenderer::DestroyFrameBuffer(bgfx::FrameBufferHandle& fbh) {
    if (bgfx::isValid(fbh)) {
        bgfx::destroy(fbh);
        fbh = BGFX_INVALID_HANDLE;
    }
}

bgfx::TextureHandle GraphicsRenderer::CreateTexture2D(uint16_t width, uint16_t height, bgfx::TextureFormat::Enum format, uint64_t flags) {
    return bgfx::createTexture2D(width, height, false, 1, format, flags);
}

void GraphicsRenderer::DestroyTexture(bgfx::TextureHandle& th) {
    if (bgfx::isValid(th)) {
        bgfx::destroy(th);
        th = BGFX_INVALID_HANDLE;
    }
}