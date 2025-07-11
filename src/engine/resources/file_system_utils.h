/*******************************************************************************
 * FILE SYSTEM UTILITIES
 * - Provides filesystem abstractions for common operations, including:
 * - path manipulation, file I/O, directory traversal, and file metadata access.
 * - Uses std::filesystem internally
 *
 * TODO: Implement missing functionality cited below:
 * - File I/O:
 *   - ReadFileLines() - Read file line by line into vector
 *   - WriteFile() - Write string to file
 *   - AppendToFile() - Append string to file
 *   - Touch() - Create empty file or update timestamp
 *
 * - Path functions:
 *   - GetAbsolutePath() - Convert to absolute path
 *   - GetCanonicalPath() - Normalize path with symbolic link resolution
 *   - NormalizeSeparators() - Standardize path separators
 *   - HasExtension() - Check if path has specific extension
 *
 * - File operations:
 *   - CopyFile() - Copy file with options
 *   - Remove() - Remove file or empty directory
 *   - RemoveAll() - Recursively remove directory and contents
 *   - Rename() - Rename/move file or directory
 *
 * - File checks:
 *   - IsRegularFile() - Check if path is a regular file
 *   - IsSymlink() - Check if path is a symbolic link
 *   - IsEmpty() - Check if directory or file is empty
 *   - FileSize() - Get file size
 *
 * - Directory operations:
 *   - CreateDirectory() - Create single directory
 *   - GetDirectoryEntries() - Get all directory entries
 *   - GetFolders() - Get only folders in directory
 *   - GetFiles() - Get only files in directory
 *   - GetFilesWithExtension(s) - Get files with specific extension(s)
 *   - GetFilesMatching() - Get files matching regex pattern
 *
 * - Time operations:
 *   - GetLastWriteTime() - Get last modification time
 *   - SetLastWriteTime() - Set last modification time
 *
 * - System & space info:
 *   - GetFreeSpace() - Get free space on drive
 *   - GetTotalSpace() - Get total capacity of drive
 *   - GetAvailableSpace() - Get available space on drive
 *   - GetTempDirectory() - Get system temp directory
 *   - CreateTempDirectory() - Create a temporary directory
 *   - CreateTempFilename() - Generate a temporary filename
 *
 * - Links:
 *   - CreateSymlink() - Create symbolic link
 *   - CreateHardLink() - Create hard link
 *   - CreateDirectorySymlink() - Create directory symbolic link
 *   - ReadSymlink() - Read symbolic link target
 ******************************************************************************/
#ifndef FILE_SYSTEM_UTILS_H
#define FILE_SYSTEM_UTILS_H

#include <engine/core/logger.h>
#include <filesystem>
#include <string>

namespace FileSystem {

using Path = std::filesystem::path;

inline bool Exists(const Path& path) {
  std::error_code ec;
  bool result = std::filesystem::exists(path, ec);
  if (ec) {
    Logger::getInstance().Log(
        LogLevel::Warning, "Error checking if path exists: " + path.string() +
                               ": " + ec.message());
    return false;
  }
  return result;
}

inline bool IsDirectory(const Path& path) {
  std::error_code ec;
  bool result = std::filesystem::is_directory(path, ec);
  if (ec) {
    Logger::getInstance().Log(LogLevel::Warning,
                              "Error checking if path is directory: " +
                                  path.string() + ": " + ec.message());
    return false;
  }
  return result;
}

inline Path GetRelativePath(const Path& path, const Path& base) {
  std::error_code ec;
  Path result = std::filesystem::relative(path, base, ec);
  if (ec) {
    Logger::getInstance().Log(
        LogLevel::Warning, "Failed to get relative path from " + base.string() +
                               " to " + path.string() + ": " + ec.message());
    return path;  // Return original path if failed
  }
  return result;
}

inline bool CreateDirectories(const Path& path) {
  std::error_code ec;
  bool result = std::filesystem::create_directories(path, ec);
  if (ec && ec.value() != 0) {  // Ignore if directory exists
    Logger::getInstance().Log(
        LogLevel::Warning,
        "Failed to create directories " + path.string() + ": " + ec.message());
    return false;
  }
  return true;
}

inline Path CurrentPath() {
  std::error_code ec;
  Path result = std::filesystem::current_path(ec);
  if (ec) {
    Logger::getInstance().Log(LogLevel::Warning,
                              "Failed to get current path: " + ec.message());
    return Path();
  }
  return result;
}

inline std::string ReadFile(const Path& path) {
  std::error_code ec;
  if (!std::filesystem::exists(path, ec)) {
    Logger::getInstance().Log(
        LogLevel::Warning,
        "FileSystem::ReadFile - Failed to find file: " + path.string());
    return "";
  }

  std::ifstream file(path, std::ios::in | std::ios::binary);
  if (!file.is_open()) {
    Logger::getInstance().Log(
        LogLevel::Warning,
        "FileSystem::ReadFile - Failed to open file: " + path.string());
    return "";
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

}  // namespace FileSystem

#endif  // FILE_SYSTEM_UTILS_H