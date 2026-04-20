#ifndef PROCMODEL_VALIDATION_LOGGER_H
#define PROCMODEL_VALIDATION_LOGGER_H

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

#include "validation_result.h"

namespace ProcModel {

// Append-only JSONL writer for ValidationResult records.
//
// One logger per session, owned by PCGEngine. Opens the file lazily on
// first Write() call; closes it in the destructor. Flushes after every
// record so interrupted sessions still produce usable data for analysis.
//
// Thread-safe: Write() is serialized with an internal mutex. Safe to call
// from any thread, though current callers are main-thread only.
class ValidationLogger {
public:
  ValidationLogger() = default;
  ~ValidationLogger();

  ValidationLogger(const ValidationLogger&) = delete;
  ValidationLogger& operator=(const ValidationLogger&) = delete;

  // Set the output file path. If called after records have been written,
  // the existing file is closed and a new one opened on next Write().
  void SetPath(const std::filesystem::path& path);

  // Write one record as a single JSON line. Returns false on I/O failure.
  bool Write(const ValidationResult& result);

  // Default path: "<cwd>/logs/procmodel/variation_log.jsonl".
  // Intended to resolve to build/<platform>/<arch>/release/logs/procmodel/
  // when the engine is launched from its build output directory.
  static std::filesystem::path DefaultLogPath();

private:
  bool EnsureOpen();

  std::filesystem::path path_;
  std::ofstream stream_;
  std::mutex mutex_;
  bool open_attempted_ = false;
};

}  // namespace ProcModel

#endif  // PROCMODEL_VALIDATION_LOGGER_H