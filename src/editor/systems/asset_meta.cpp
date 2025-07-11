#include "asset_meta.h"
#include <iomanip>
#include <random>
#include <sstream>
#include "engine/core/logger.h"

// Generate a simple GUID string (UUID v4 format)
// Used by AssetMeta::ParseGuid
// TODO: move this to engine/utilities/guid.h instead
std::string GenerateGuid() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 15);
  static std::uniform_int_distribution<> dis2(8, 11);

  std::stringstream ss;
  ss << std::hex;

  for (int i = 0; i < 8; i++) {
    ss << dis(gen);
  }
  ss << "-";

  for (int i = 0; i < 4; i++) {
    ss << dis(gen);
  }
  ss << "-4";  // Version 4

  for (int i = 0; i < 3; i++) {
    ss << dis(gen);
  }
  ss << "-";

  ss << dis2(gen);
  for (int i = 0; i < 3; i++) {
    ss << dis(gen);
  }
  ss << "-";

  for (int i = 0; i < 12; i++) {
    ss << dis(gen);
  }

  return ss.str();
}

namespace AssetMeta {

AssetGuid ParseGuid(const std::filesystem::path& path) {
  try {
    // Ensure metadata file exists
    if (!std::filesystem::exists(path))
      return GenerateGuid();

    // Open metadata file
    std::ifstream file(path);
    if (!file.is_open()) {
      return GenerateGuid();
    }

    // Read first line of metadata file (containing GUID)
    std::string line;
    if (!std::getline(file, line))
      return GenerateGuid();

    // Extract GUID string
    const std::string guid_prefix = "guid: ";
    if (line.rfind(guid_prefix, 0) != 0)
      return GenerateGuid();

    std::string guid_str = line.substr(guid_prefix.length());
    return guid_str;
  } catch (...) {
    return GenerateGuid();
  }
}

std::string CreateHeader(const AssetGuid& guid) {
  return "guid: " + guid + "\n---\n";
}

void RemoveHeader(std::string& source) {
  try {
    // Find header document separator
    size_t separator_pos = source.find("---");
    if (separator_pos == std::string::npos)
      return;

    size_t newline_after_separator = source.find('\n', separator_pos);
    if (newline_after_separator == std::string::npos)
      return;

    // Remove header
    source = source.substr(newline_after_separator + 1);
  } catch (const std::exception& e) {
    Logger::getInstance().Log(
        LogLevel::Warning,
        "Failed to remove metadata header: " + std::string(e.what()));
  }
}

}  // namespace AssetMeta