#ifndef PCG_OPERATION_REGISTRY_H
#define PCG_OPERATION_REGISTRY_H

#include <functional>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <unordered_map>

#include "engine/pcg/pipeline/operation_context.h"
#include "engine/pcg/pipeline/operation_data.h"

namespace PCG {

class OperationRegistry {
public:
  using Parser =
      std::function<std::shared_ptr<OperationData>(const nlohmann::json&)>;
  using Handler = std::function<void(const OperationData&, OperationContext&)>;

  OperationRegistry() = default;
  ~OperationRegistry() = default;

  // Register new operation kind. Both parser and handler must be provided.
  // Re-registering the same kind overwrites previous entry.
  void Register(std::string kind, Parser parser, Handler handler);

  // Parse params for registered kind. Returns nullptr on unknown kind or
  // parser failure (parsers should return nullptr to signal invalid params).
  std::shared_ptr<OperationData> Parse(const std::string& kind,
                                       const nlohmann::json& params) const;

  // Apply a previously-parsed operation. Returns false if kind not registered.
  bool Apply(const std::string& kind, const OperationData& data,
             OperationContext& ctx) const;

  bool IsRegistered(const std::string& kind) const;

  // Look up handler directly. Used by pipeline compilation to bind handlers
  // at compile time, avoiding name lookup in the hot path.
  Handler GetHandler(const std::string& kind) const;

private:
  struct Entry {
    Parser parse;
    Handler apply;
  };

  std::unordered_map<std::string, Entry> entries_;
};

}  // namespace PCG

#endif  // PCG_OPERATION_REGISTRY_H