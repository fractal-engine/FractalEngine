#ifndef MODEL_IMPORT_H
#define MODEL_IMPORT_H

#include <string>

namespace GltfImport {
void SetupGltfLayouts();
void LoadModelAndSpawn(const std::string& filepath);
}  // namespace GltfImport

#endif  // MODEL_IMPORT_H
