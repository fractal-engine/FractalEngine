#ifndef PLATFORM_PATHS_H
#define PLATFORM_PATHS_H

#include <string>

namespace Platform {
// Return absolute directory containing running executable
const std::string& GetExecutableDir();

// Resolve a relative asset path against executable directory
std::string ResolvePath(const std::string& relative);
}  // namespace Platform

#endif  // PLATFORM_PATHS_H