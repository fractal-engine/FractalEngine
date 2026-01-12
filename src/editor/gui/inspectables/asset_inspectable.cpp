#include "asset_inspectable.h"

#include "editor/runtime/runtime.h"

AssetInspectable::AssetInspectable(AssetSID asset_id) : asset_id_(asset_id) {
  // store SID
}

// Draw once when InspectableBase is opened
void AssetInspectable::RenderStaticContent(ImDrawList& draw_list) {
  if (auto asset = Runtime::Project().Assets().Get(asset_id_)) {
    asset->DrawInspectorUI();
  } else {
    ImGui::TextDisabled("Asset not found.");
  }
}

void AssetInspectable::RenderDynamicContent(ImDrawList& draw_list) {
  // TODO: Nothing animated yet
}
