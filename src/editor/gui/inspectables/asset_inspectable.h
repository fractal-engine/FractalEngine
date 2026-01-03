#ifndef ASSET_INSPECTABLE_H
#define ASSET_INSPECTABLE_H

#include "editor/systems/editor_asset.h"
#include "inspectable_base.h"

class AssetInspectable : public InspectableBase {
public:
  AssetInspectable(AssetSID asset_id);

  void RenderStaticContent(ImDrawList& draw_list) override;
  void RenderDynamicContent(ImDrawList& draw_list) override;

private:
  AssetSID asset_id_;
};

#endif  // ASSET_INSPECTABLE_H