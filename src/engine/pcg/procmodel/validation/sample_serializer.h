#ifndef SAMPLE_SERIALIZER_H
#define SAMPLE_SERIALIZER_H

#include <nlohmann/json.hpp>

#include "engine/pcg/procmodel/validation/procmodel_analysis.h"

namespace ProcModel {

nlohmann::json SerializeSample(const ProcModelSample& sample);

}  // namespace ProcModel

#endif  // SAMPLE_SERIALIZER_H