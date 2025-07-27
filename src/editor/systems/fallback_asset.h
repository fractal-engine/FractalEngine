#ifndef FALLBACK_ASSET_H
#define FALLBACK_ASSET_H

#include "editor/systems/editor_asset.h"
#include "engine/core/logger.h"

class FallbackAsset : public EditorAsset {
public:
  struct Meta {
    bool empty_ = true;
  };

  FallbackAsset();
  ~FallbackAsset() override;

  void OnDefaultLoad(const std::filesystem::path& meta_path) override;
  void OnUnload() override;
  void OnReload() override;
  void DrawInspectorUI() override;
  uint32_t GetIcon() const override;

private:
  Meta meta_;
};

#endif  // FALLBACK_ASSET_H