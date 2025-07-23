#ifndef GLTF_TRANSLATOR_H
#define GLTF_TRANSLATOR_H

#include <string>
#include <vector>

#include "engine/resources/3d/mesh_data.h"

namespace Formats {

// Return mesh data per glTF primitive
std::vector<Resources3D::MeshData> TranslateGLTF(const std::string& path);

}  // namespace Formats

#endif  // GLTF_TRANSLATOR_H
