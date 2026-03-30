#ifndef PROCMODEL_RESOURCE_H
#define PROCMODEL_RESOURCE_H

#include <string>

#include "engine/memory/resource.h"
#include "engine/pcg/procmodel/descriptor/model_descriptor.h"
#include "engine/pcg/procmodel/generator/resolved_model.h"
#include "engine/pcg/procmodel/model_graph/model_graph.h"

namespace ProcModel {

// Wrap archetype pipeline state
class ProcModelResource : public Resource {
public:
  ProcModelResource() = default;

  ModelGraph& GetGraph() { return graph_; }
  const ModelGraph& GetGraph() const { return graph_; }

  ModelDescriptor& GetDescriptor() { return descriptor_; }
  const ModelDescriptor& GetDescriptor() const { return descriptor_; }

  bool IsResolved() const { return resolved_; }
  void SetResolved(bool value) { resolved_ = value; }

  void Destroy() override {
    graph_ = {};
    descriptor_ = {};
    resolved_ = false;
  }

  std::string GetName() const { return descriptor_.model_id; }

private:
  ModelGraph graph_;
  ModelDescriptor descriptor_;
  bool resolved_ = false;
};

}  // namespace ProcModel

#endif  // PROCMODEL_RESOURCE_H
