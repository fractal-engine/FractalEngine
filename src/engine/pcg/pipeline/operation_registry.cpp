#include "operation_registry.h"

#include <utility>

namespace PCG {

void OperationRegistry::Register(std::string kind, Parser parser,
                                 Handler handler) {
  entries_[std::move(kind)] = Entry{std::move(parser), std::move(handler)};
}

std::shared_ptr<OperationData> OperationRegistry::Parse(
    const std::string& kind, const nlohmann::json& params) const {
  auto it = entries_.find(kind);
  if (it == entries_.end()) {
    return nullptr;
  }
  return it->second.parse(params);
}

bool OperationRegistry::Apply(const std::string& kind,
                              const OperationData& data,
                              OperationContext& ctx) const {
  auto it = entries_.find(kind);
  if (it == entries_.end()) {
    return false;
  }
  it->second.apply(data, ctx);
  return true;
}

bool OperationRegistry::IsRegistered(const std::string& kind) const {
  return entries_.find(kind) != entries_.end();
}

OperationRegistry::Handler OperationRegistry::GetHandler(
    const std::string& kind) const {
  auto it = entries_.find(kind);
  if (it == entries_.end()) {
    return nullptr;
  }
  return it->second.apply;
}

}  // namespace PCG