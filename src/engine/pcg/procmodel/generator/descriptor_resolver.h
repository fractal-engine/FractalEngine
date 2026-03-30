#include <string>
#include <vector>

#include "engine/pcg/procmodel/descriptor/model_descriptor.h"
#include "engine/pcg/procmodel/model_graph/model_graph.h"

namespace ProcModel {

class DescriptorResolver {
public:
  struct ResolveResult {
    bool success = false;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
  };

  static ResolveResult Resolve(ModelGraph& graph,
                               const ModelDescriptor& descriptor);

private:
  static bool MapSelectionGroups(ModelGraph& graph,
                                 const ModelDescriptor& descriptor,
                                 std::vector<std::string>& errors);

  static bool MapParameterRanges(ModelGraph& graph,
                                 const ModelDescriptor& descriptor,
                                 std::vector<std::string>& errors);

  static bool MapParameterBindings(ModelGraph& graph,
                                   const ModelDescriptor& descriptor,
                                   std::vector<std::string>& errors);
};

}  // namespace ProcModel
