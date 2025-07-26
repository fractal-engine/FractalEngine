#ifndef ASSET_META_H
#define ASSET_META_H

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <sstream>
#include <string>

#include "engine/core/logger.h"
#include "engine/resources/file_system_utils.h"
#include "engine/resources/guid.h"

// Global persistent asset identifier
using AssetGuid = XG::GUID;

// Session-only runtime identifier for assets
using AssetSID = uint32_t;

// Combined key for asset lookups
struct AssetKey {
  AssetGuid guid_;
  AssetSID sid_ = 0;
};

namespace AssetMeta {

// Returns the GUID from a metadata file header
AssetGuid ParseGuid(const std::filesystem::path& path);

// Create a metadata file header with GUID
std::string CreateHeader(AssetGuid guid);

// Remove header portion from metadata file content
void RemoveHeader(std::string& source);

// Serialize an object to a metadata file
template <typename T>
bool Serialize(const T& obj, const std::filesystem::path& path) {
  std::string serialised;

  // Serialize object
  try {
    serialised = rfl::json::write(obj);
    serialised = nlohmann::json::parse(serialised).dump(4);
  } catch (const std::exception& e) {
    Logger::getInstance().Log(
        LogLevel::Warning, "AssetMeta::Serialize - " + std::string(e.what()));
    return false;
  }

  // Add metadata header with GUID
  serialised.insert(0, CreateHeader(ParseGuid(path)));

  // Write serialised data to file
  std::ofstream f(path);
  if (!f) {
    Logger::getInstance().Log(
        LogLevel::Error, "AssetMeta::Serialize - cannot open " + path.string());
    return false;
  }
  f << serialised;
  return true;
}

// Deserialize a metadata file to an object
template <typename T>
std::optional<T> Deserialize(const std::filesystem::path& path) {
  // Check if file exists
  std::string source = FileSystem::ReadFile(path);
  if (source.empty())
    return std::nullopt;

  // Remove header from metadata source
  RemoveHeader(source);
  if (source.empty())
    return std::nullopt;

  try {
    const T obj = rfl::json::read<T>(source).value(); // try to parse json to Result<T>
    return obj; 

  } catch (const std::exception& e) {
    Logger::getInstance().Log(
        LogLevel::Warning, "AssetMeta::Deserialize - " + std::string(e.what()));
    return std::nullopt;
  }
}

// Load metadata from file, create default if missing
template <typename T>
void LoadMeta(T* slot, const std::filesystem::path& path) {
  auto meta = Deserialize<T>(path);
  if (!meta) {
    *slot = T();
    Serialize(*slot, path);
  } else {
    *slot = meta.value();
  }
}

}  // namespace AssetMeta

#endif  // ASSET_META_H