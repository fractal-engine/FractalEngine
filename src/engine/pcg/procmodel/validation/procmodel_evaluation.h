#ifndef PROCMODEL_EVALUATION_H
#define PROCMODEL_EVALUATION_H

#include <vector>

#include "engine/pcg/procmodel/validation/procmodel_analysis.h"

namespace ProcModel {

class ProcModelEval {
public:
  // Analyze a batch of samples and produce ERA metrics.
  // Only passed samples are included in the analysis.
  static ProcModelERA Compute(const std::vector<ProcModelSample>& samples);

  static bool WriteReport(const ProcModelERA& era, const std::string& path);
};

}  // namespace ProcModel

#endif  // PROCMODEL_EVALUATION_H