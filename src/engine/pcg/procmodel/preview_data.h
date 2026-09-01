#ifndef PREVIEW_DATA_H
#define PREVIEW_DATA_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/memory/resource.h"
#include "engine/pcg/procmodel/generator/resolved_model.h"

class Model;

struct PreviewData {
  std::shared_ptr<Model> model;
  ResourceID archetype_id = 0;
  std::vector<ProcModel::InstanceModel> instances;
  std::vector<std::shared_ptr<Model>> instance_model_data;
  int selected_instance = 0;
};

#endif  // PREVIEW_DATA_H
