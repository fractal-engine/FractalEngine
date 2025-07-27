#include "asset_meta.h"
#include <iomanip>
#include <random>
#include <sstream>
#include "engine/core/logger.h"

namespace AssetMeta {

AssetGuid ParseGuid(const std::filesystem::path& path) {
  try {
    // Ensure metadata file exists
    if (!std::filesystem::exists(path))
      return AssetGuid();

    // Open metadata file
    std::ifstream file(path);
    if (!file.is_open()) {
      return AssetGuid();
    }

    // Read first line of metadata file (containing GUID)
    std::string line;
    if (!std::getline(file, line))
      return AssetGuid();

    // Extract GUID string
    const std::string guid_prefix = "guid: ";
    if (line.rfind(guid_prefix, 0) != 0)
      return AssetGuid();

    std::string guid_str = line.substr(guid_prefix.length());
    return AssetGuid(guid_str);
  } catch (...) {
    return AssetGuid();
  }
}

std::string CreateHeader(AssetGuid guid) {
  return "guid: " + guid.str() + "\n---\n";
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