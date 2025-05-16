#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>  // file modification lib
#include <iostream>
#include <map>     // for log level mapping
#include <memory>  // smart pointer
#include <mutex>   // thread sync
#include <string>
#include <vector>

#include "core/singleton.hpp"
// Enumeration for log levels
enum class LogLevel { Info, Debug, Warning, Error };

class Logger : public Singleton<Logger> {
public:
  void Log(LogLevel level, const std::string& message);  // Log message
  std::vector<std::string> GetLogEntries();              // Get log entries

  Logger();
  ~Logger();

  std::ofstream logfile_;  // stream data to file
  std::mutex mutex_;  // support for multi-thread i.e. only 1 thread can access
                      // at a time
  std::map<LogLevel, std::string> logLevelNames{{LogLevel::Info, "INFO"},
                                                {LogLevel::Debug, "DEBUG"},
                                                {LogLevel::Warning, "WARNING"},
                                                {LogLevel::Error, "ERROR"}};

  std::string GetLogLevelName(LogLevel level);
};

#endif  // LOGGER_H
