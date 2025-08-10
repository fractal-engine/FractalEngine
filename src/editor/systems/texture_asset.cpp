/***************************************************************************************************
 * TODO: TextureAsset
 *
 * We already have:
 *  -- Gfx::Texture / TextureCache:
 *      – loads PNG/JPG files and keeps a shared_ptr to the GPU handle
 *  -- IconLoader:
 *      – central registry that turns small textures into ImGui icons
 *  -- AssetBrowserPanel:
 *      - UI for browsing files, shows folders & files and can display an
 *ImGui::Image
 *
 * TextureAsset should eventually implement the following:
 * 1. **Meta-file import options**
 *      - Read a side-car “foo.png.meta” file that stores import settings
 *        (intended TextureType, sRGB flag, normal-map invert, mip-map options,
 *etc.)
 *      - Persist this data to `meta` so that re-imports remain deterministic.
 *
 * 2. **Centralised lifetime management**
 *      - After introducing a *ResourceManager*, `TextureAsset` should become
 * its public handle. The manager keeps one GPU copy per asset and
 * reference-counts it across scenes, materials, inspector previews, etc.
 *      - `onUnload()` must then return the GPU resource to the manager
 * instead of just letting a shared_ptr drop.
 *
 * 3. **Hot-reload**
 *      - `onReload()` triggers when the source file or its meta changes on
 * disk; recreate the GPU texture in place and signal dependent materials to
 * rebind.
 *
 * 4. **Inspector / thumbnail helpers**
 *      - `icon()` returns a small IconLoader handle for grids;
 *`renderInspectableUI()` draws a big preview in the property panel. Both
 * simply forward the GPU id they own.
 *
 * 5. **Editor-only convenience API**
 *      - `load(TextureType type)` should be a thin wrapper that calls the
 * ResourceManager, waits (sync or async) for completion and caches
 * `textureResource` for UI access.
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────────
 * CURRENT STATE:
 * - AssetBrowser can currently loads raw thumbnails directly via
 *`TextureCache::Instance().Get(path, Gfx::TextureType::IMAGE)`
 * - ResourceManager is not implemented yet.
 * - InspectorPanel is not implemented yet.
 * - No hot-reload support yet.
 **************************************************************************************************/

#include "texture_asset.h"
#include "engine/core/logger.h"

TextureAsset::TextureAsset() : meta_() {}

TextureAsset::~TextureAsset() {}

// ──────────────────────────────────────────────────────────────────────────────
//  Ctors / loading
// ──────────────────────────────────────────────────────────────────────────────
TextureAsset::TextureAsset(std::filesystem::path file)
    : file_path_(std::move(file)) {
  LoadSync();  // immediate, blocking load (used for editor thumbnail)
}

void TextureAsset::LoadSync() {
  // direct call into the global TextureCache
  texture_ =
      TextureCache::Instance().Get(file_path_, TextureType::IMAGE);

  if (!texture_ || !texture_->Valid()) {
    Logger::getInstance().Log(
        LogLevel::Warning, "TextureAsset: failed to load '" +
                               file_path_.string() + "', using fallback icon");
  }
}

// ──────────────────────────────────────────────────────────────────────────────
//  Public helpers
// ──────────────────────────────────────────────────────────────────────────────
uint32_t TextureAsset::GpuHandle() const {
  if (texture_ && texture_->Valid())
    return texture_->Handle().idx;

  // fallback icon
  return IconLoader::GetIconHandle("texture");
}

void TextureAsset::DrawInspectorUI() {
  ImGui::TextUnformatted("TODO: texture inspector UI");

  // quick preview if there's a texture
  if (texture_ && texture_->Valid()) {
    float side = ImGui::GetContentRegionAvail().x;
    ImGui::Image(IconLoader::ToImGuiTexture("texture"), {side, side});
  }
}

void TextureAsset::Reload() {
  // TODO: use Resource-Manager's hot reload instead
  LoadSync();
}
