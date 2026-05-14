#ifndef IO_JSON_H
#define IO_JSON_H

#include <nlohmann/json.hpp>

#include <optional>
#include <sstream>
#include <string>

namespace Content {

// Reads a JSON file from disk and parses it into a nlohmann::json.
// Returns std::nullopt on I/O failure or JSON parse error (logged).
//
// This is the file -> JSON boundary. Callers downstream interpret the
// JSON shape into their own structs
std::optional<nlohmann::json> ReadJsonFile(const std::string& path);

// Writes a JSON object to disk at the given path. Uses pretty-printing
// with 2-space indentation by default; pass indent < 0 for minified output.
// Returns false on I/O failure (logged).
bool WriteJsonFile(const std::string& path, const nlohmann::json& j,
                   int indent = 2);

}  // namespace Content

#endif  // IO_JSON_H