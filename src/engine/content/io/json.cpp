#include "json.h"

#include "engine/core/file_system_utils.h"
#include "engine/core/logger.h"

namespace Content {

std::optional<nlohmann::json> ReadJsonFile(const std::string& path) {
  if (!FileSystem::Exists(path)) {
    Logger::getInstance().Log(
        LogLevel::Error, "[Content::ReadJsonFile] File not found: " + path);
    return std::nullopt;
  }

  std::string contents = FileSystem::ReadFile(path);
  if (contents.empty()) {
    // Log open failure if it occurred.
    // Empty files are also invalid JSON; parser will reject it.
    Logger::getInstance().Log(
        LogLevel::Error,
        "[Content::ReadJsonFile] Empty or unreadable: " + path);
    return std::nullopt;
  }

  try {
    return nlohmann::json::parse(contents);
  } catch (const nlohmann::json::parse_error& e) {
    Logger::getInstance().Log(
        LogLevel::Error,
        std::string("[Content::ReadJsonFile] Parse error in ") + path + ": " +
            e.what());
    return std::nullopt;
  }
}

bool WriteJsonFile(const std::string& path, const nlohmann::json& j,
                   int indent) {
  try {
    std::ofstream file(path);
    if (!file.is_open()) {
      Logger::getInstance().Log(
          LogLevel::Error,
          "[Content::WriteJsonFile] Cannot open for writing: " + path);
      return false;
    }
    if (indent < 0) {
      file << j.dump();
    } else {
      file << j.dump(indent);
    }
    return file.good();
  } catch (const std::exception& e) {
    Logger::getInstance().Log(
        LogLevel::Error,
        std::string("[Content::WriteJsonFile] Serialization error in ") + path +
            ": " + e.what());
    return false;
  }
}

}  // namespace Content