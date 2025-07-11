#ifndef PROJECT_MANAGER_H
#define PROJECT_MANAGER_H

#include <filesystem>
#include <string>
#include "project_assets.h"
#include "project_observer.h"

struct Project {
  std::filesystem::path path_;
};

class ProjectManager {
public:

ProjectManager();

// Poll for file system events
void PollEvents();

// Load project from specified directory
bool Load(const std::filesystem::path& directory);

// Accessors
const Project& GetProject() const;
ProjectObserver& GetObserver();
ProjectAssets& Assets();

// Path utility, resolves path relative to project root
std::filesystem::path AbsolutePath(const std::filesystem::path& path);

private:

// Ensure project configuration exists
bool EnsureConfig();

Project project_;
ProjectObserver observer_;
ProjectAssets assets_;
};

#endif  // PROJECT_MANAGER_H