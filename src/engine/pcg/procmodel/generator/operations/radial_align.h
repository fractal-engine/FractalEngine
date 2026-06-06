#ifndef RADIAL_ALIGN_H
#define RADIAL_ALIGN_H

#include "engine/pcg/pipeline/operation_data.h"
#include "engine/pcg/pipeline/operation_registry.h"

namespace ProcModel {

// Rebuilds a locator's rotation so its outward axis points from the
// activator's origin to the locator's position. Position is preserved.
// Useful for radially symmetric structures (trunks, towers) where locators
// were authored at correct positions but with uniform local rotation.
struct RadialAlignData : public PCG::OperationData {};

void RegisterRadialAlignOperation(PCG::OperationRegistry& registry);

}  // namespace ProcModel

#endif  // RADIAL_ALIGN_H