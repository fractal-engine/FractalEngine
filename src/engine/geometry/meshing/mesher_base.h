#ifndef MESHER_BASE_H
#define MESHER_BASE_H

#include "engine/core/types/geometry_data.h"

namespace Geometry {

class MesherBase {
public:
  virtual ~MesherBase() = default;
  virtual MeshData Generate() const = 0;
};

}  // namespace Geometry

#endif  // MESHER_BASE_H