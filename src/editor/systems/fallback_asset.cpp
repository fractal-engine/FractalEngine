#include "fallback_asset.h"

#include <imgui.h>

#include "engine/core/logger.h"
#include "engine/renderer/icons/icon_loader.h"

FallbackAsset::FallbackAsset() : meta_() {}

FallbackAsset::~FallbackAsset() {}

void FallbackAsset::OnDefaultLoad(const std::filesystem::path& meta_path) {
  Logger::getInstance().Log(LogLevel::Info,
                            "FallbackAsset: Loaded from " + meta_path.string());

  // Load metadata
  AssetMeta::LoadMeta<Meta>(&meta_, meta_path);
}

void FallbackAsset::OnUnload() {
  // Nothing specific to unload
  Logger::getInstance().Log(LogLevel::Info, "FallbackAsset: Unloaded");
}

void FallbackAsset::OnReload() {
  // Nothing specific to reload
  Logger::getInstance().Log(LogLevel::Info, "FallbackAsset: Reloaded");
}

void FallbackAsset::DrawInspectorUI() {
  ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Fallback Asset");
  ImGui::Separator();

  ImGui::Text("Asset: %s", GetPath().filename().string().c_str());
  ImGui::TextWrapped("This is a generic asset with no specific editor UI.");
}

uint32_t FallbackAsset::GetIcon() const {
  return IconLoader::GetIconHandle("file");
}