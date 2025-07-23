#include "project_manager.h"
#include <fstream>
#include "engine/core/logger.h"

ProjectManager::ProjectManager() : project_(), observer_(), assets_() {}

void ProjectManager::PollEvents() {
  observer_.PollEvents();
}

bool ProjectManager::Load(const std::filesystem::path& directory) {
  if (!std::filesystem::exists(directory) ||
      !std::filesystem::is_directory(directory)) {
    Logger::getInstance().Log(LogLevel::Error,
                              "Project path is invalid. Provided path was '" +
                                  directory.string() + "'");
    return false;
  }

  // Set project path
  project_.path_ = directory;

  // Ensure project configuration is valid
  if (!EnsureConfig()) {
    Logger::getInstance().Log(LogLevel::Error,
                              "Failed to ensure project configuration.");
    return false;
  }
  // if true, start observing project
  observer_.SetTarget(project_.path_);
  return true;
}

const Project& ProjectManager::GetProject() const {
  return project_;
}

ProjectObserver& ProjectManager::GetObserver() {
  return observer_;
}

ProjectAssets& ProjectManager::Assets() {
  return assets_;
}

std::filesystem::path ProjectManager::AbsolutePath(
    const std::filesystem::path& path) {
  return project_.path_ / path;
}

bool ProjectManager::EnsureConfig() {

  // Default config path
  std::filesystem::path config_path = project_.path_ / ".project";

  // Check if config exists, or try to create it
  if (std::filesystem::exists(config_path))
    return true;

  try {
    std::ofstream config_file(config_path);
    if (config_file.is_open()) {
      config_file << "# Project configuration file" << std::endl;
      config_file.close();
      return true;
    }
  } catch (const std::exception& e) {
    Logger::getInstance().Log(
        LogLevel::Error,
        "Failed to create project configuration: " + std::string(e.what()));
  }

  Logger::getInstance().Log(
      LogLevel::Error, "Could not locate or create project configuration at '" +
                           config_path.string() + "'");
  return false;
}

// TODO: rework function, project name and
// folder name should be handled separetely
std::string ProjectManager::ProjectName() const
{
    return project_.path_.filename().string();
}

