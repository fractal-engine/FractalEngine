#include "validation_logger.h"

#include <nlohmann/json.hpp>
#include <system_error>

#include "engine/core/logger.h"

namespace ProcModel {

// ---------------------------------------------------------------------------
// JSON serialization
// ---------------------------------------------------------------------------
static nlohmann::json SerializeAABB(const AABB& box) {
  nlohmann::json j;
  j["valid"] = box.valid;
  if (box.valid) {
    j["min"] = {box.min.x, box.min.y, box.min.z};
    j["max"] = {box.max.x, box.max.y, box.max.z};
  }
  return j;
}

static nlohmann::json SerializeDiagnostic(const Diagnostic& d) {
  nlohmann::json j;
  j["code"] = d.code;
  j["severity"] =
      d.severity == Diagnostic::Severity::Error ? "error" : "warning";
  j["part_ids"] = d.part_ids;
  if (!d.group_id.empty())
    j["group_id"] = d.group_id;
  j["message"] = d.message;
  return j;
}

static nlohmann::json SerializeResolvedEntry(
    const ValidationResult::ResolvedEntry& e) {
  nlohmann::json j;
  j["descriptor_id"] = e.descriptor_id;
  j["group_id"] = e.group_id;
  j["applied_rotation"] = {e.applied_rotation.x, e.applied_rotation.y,
                           e.applied_rotation.z};
  j["applied_scale"] = {e.applied_scale.x, e.applied_scale.y,
                        e.applied_scale.z};
  j["attach_to"] = e.attach_to;
  return j;
}

static nlohmann::json SerializeResult(const ValidationResult& r) {
  nlohmann::json j;

  // Identity
  j["model_id"] = r.model_id;
  j["seed"] = r.seed;
  j["attempt_index"] = r.attempt_index;
  j["timestamp_ms"] = r.timestamp_ms;

  // Outcome
  j["passed"] = r.passed;

  nlohmann::json diags = nlohmann::json::array();
  for (const auto& d : r.diagnostics) {
    diags.push_back(SerializeDiagnostic(d));
  }
  j["diagnostics"] = std::move(diags);

  // Raw selection data
  j["selected_part_ids"] = r.selected_part_ids;
  j["active_group_ids"] = r.active_group_ids;

  // Raw parameter data
  nlohmann::json entries = nlohmann::json::array();
  for (const auto& e : r.resolved_entries) {
    entries.push_back(SerializeResolvedEntry(e));
  }
  j["resolved_entries"] = std::move(entries);

  // Geometry
  j["model_bounds"] = SerializeAABB(r.model_bounds);

  nlohmann::json per_part = nlohmann::json::object();
  for (const auto& [id, box] : r.per_part_bounds) {
    per_part[id] = SerializeAABB(box);
  }
  j["per_part_bounds"] = std::move(per_part);

  nlohmann::json per_group = nlohmann::json::object();
  for (const auto& [id, box] : r.per_group_bounds) {
    per_group[id] = SerializeAABB(box);
  }
  j["per_group_bounds"] = std::move(per_group);

  return j;
}

// ---------------------------------------------------------------------------
// ValidationLogger
// ---------------------------------------------------------------------------
ValidationLogger::~ValidationLogger() {
  std::scoped_lock lock(mutex_);
  if (stream_.is_open()) {
    stream_.flush();
    stream_.close();
  }
}

void ValidationLogger::SetPath(const std::filesystem::path& path) {
  std::scoped_lock lock(mutex_);
  if (stream_.is_open()) {
    stream_.flush();
    stream_.close();
  }
  path_ = path;
  open_attempted_ = false;
}

std::filesystem::path ValidationLogger::DefaultLogPath() {
  // Resolves to the working directory at process start, which for xmake
  // builds is typically build/<platform>/<arch>/release/.
  // TODO: switch to executable-relative path once a platform abstraction
  // for exe-dir exists.
  return std::filesystem::current_path() / "logs" / "procmodel" /
         "variation_log.jsonl";
}

bool ValidationLogger::EnsureOpen() {
  if (stream_.is_open())
    return true;
  if (open_attempted_)
    return false;  // Already tried and failed this session

  open_attempted_ = true;

  if (path_.empty())
    path_ = DefaultLogPath();

  std::error_code ec;
  std::filesystem::create_directories(path_.parent_path(), ec);
  if (ec) {
    Logger::getInstance().Log(
        LogLevel::Error,
        "[ValidationLogger] Failed to create log directory: " + ec.message());
    return false;
  }

  stream_.open(path_, std::ios::out | std::ios::app);
  if (!stream_.is_open()) {
    Logger::getInstance().Log(
        LogLevel::Error,
        "[ValidationLogger] Failed to open log file: " + path_.string());
    return false;
  }

  Logger::getInstance().Log(LogLevel::Info,
                            "[ValidationLogger] Logging to " + path_.string());
  return true;
}

bool ValidationLogger::Write(const ValidationResult& result) {
  std::scoped_lock lock(mutex_);
  if (!EnsureOpen())
    return false;

  try {
    stream_ << SerializeResult(result).dump() << '\n';
    stream_.flush();
    return stream_.good();
  } catch (const std::exception& e) {
    Logger::getInstance().Log(
        LogLevel::Error,
        std::string("[ValidationLogger] Serialization failed: ") + e.what());
    return false;
  }
}

}  // namespace ProcModel