#include "core/logger.h"
#include <filesystem>

Logger::Logger() : logfile_() {

  // TODO: Improve logic here
  // Logger only stores 2 consecutive iterations of log files
  // overwrites old log with new one as a new session is started
  namespace fs = std::filesystem;

  // --- log checker ----------------------------------
  if (fs::exists("oldlog.txt"))
    fs::remove("oldlog.txt");
  if (fs::exists("enginelog.txt"))
    fs::rename("enginelog.txt", "oldlog.txt");
  // -----Open new log---------------------------------
  logfile_.open("enginelog.txt", std::ios::trunc);
  if (!logfile_)
    std::cerr << "Log File failed to open\n";
  else
    std::cerr << "Log File successfully opened at enginelog.txt\n";
}

Logger::~Logger() {  // destructor to close logger class if it is still open
                     // after execution
  if (logfile_) {
    logfile_.close();
  }
}
std::string Logger::GetLogLevelName(LogLevel level) {
  return logLevelNames[level];  // Lookup the mapping for levels
}
std::vector<std::string> Logger::GetLogEntries() {
  std::lock_guard<std::mutex> guard(mutex_);  // Lock to prevent overwriting
  std::vector<std::string> log_entries;
  std::ifstream logfile("enginelog.txt");
  if (!logfile.is_open()) {
    log_entries.emplace_back("Unable to open log file.");
    return log_entries;
  }
  std::string line;
  while (std::getline(logfile, line)) {
    log_entries.emplace_back(line);
  }
  return log_entries;
}

void Logger::Log(LogLevel level, const std::string& message) {
  std::lock_guard<std::mutex> guard(
      mutex_);  // Lock to prevent overwriting; mutex gives multi-threading
                // support
  logfile_ << "[" << GetLogLevelName(level) << "] " << message << std::endl;
  logfile_.flush();  // Ensure the message is written to the file
}
