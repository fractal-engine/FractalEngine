#include <filesystem>

#include "paths.h"

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <climits>
#elif defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#include <climits>
#endif

namespace Platform {

static std::string ComputeExecutableDir() {
#ifdef __APPLE__
  char buf[PATH_MAX];
  uint32_t size = sizeof(buf);
  if (_NSGetExecutablePath(buf, &size) == 0) {
    return std::filesystem::canonical(buf).parent_path().string();
  }
#elif defined(_WIN32)
  char buf[MAX_PATH];
  DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
  if (len > 0 && len < MAX_PATH) {
    return std::filesystem::path(buf).parent_path().string();
  }
#elif defined(__linux__)
  char buf[PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (len != -1) {
    buf[len] = '\0';
    return std::filesystem::path(buf).parent_path().string();
  }
#endif
  return std::filesystem::current_path().string();
}

const std::string& GetExecutableDir() {
  static const std::string dir = ComputeExecutableDir();
  return dir;
}

std::string ResolvePath(const std::string& relative) {
  return (std::filesystem::path(GetExecutableDir()) / relative).string();
}

}  // namespace Platform