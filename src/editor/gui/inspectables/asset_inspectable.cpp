#include "asset_inspectable.h"

#include "editor/runtime/application.h"
#include "editor/systems/editor_asset.h"
#include "engine/core/logger.h"

// -----------------------------------------------------------------------------
//  CTOR
// -----------------------------------------------------------------------------
AssetInspectable::AssetInspectable(AssetSID asset_id) : asset_id_(asset_id) {
  // store the SID
}

// -----------------------------------------------------------------------------
//  STATIC --- draw sonce when Inspectable is opened
// -----------------------------------------------------------------------------
void AssetInspectable::RenderStaticContent(ImDrawList& draw_list) {
  // Look up the asset instance that owns this SID
  auto& pm = Application::Project();  // singleton accessor
  auto& aset = pm.Assets();           // ProjectAssets instance
  auto ref = aset.Get(asset_id_);

  if (ref) {
    // Delegate actual UI to the asset itself
    ref->DrawInspectorUI();  // virtual EditorAsset
  } else {
    Logger::getInstance().Log(
        LogLevel::Warning,
        "AssetInspectable: invalid SID " + std::to_string(asset_id_));
    ImGui::TextDisabled("Asset not found.");
  }
}

// -----------------------------------------------------------------------------
//  DYNAMIC --- per-frame animated content
// -----------------------------------------------------------------------------
void AssetInspectable::RenderDynamicContent(ImDrawList& /*draw_list*/) {
  /* Nothing animated yet */
}
