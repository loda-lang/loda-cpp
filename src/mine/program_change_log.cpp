#include "mine/program_change_log.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>

#include "base/uid.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"

ProgramChangeLog::ProgramChangeLog() {
  // Only create the log file in server mode
  if (Setup::getMiningMode() == MINING_MODE_SERVER) {
    log_file_path = Setup::getDebugHome() + "programs_changelog.txt";
    log_stream.open(log_file_path, std::ios::app);
    if (!log_stream.good()) {
      // Log warning but continue - logging is non-critical
      Log::get().warn("Failed to open program change log file: " +
                      log_file_path);
      log_stream.close();  // Ensure stream is in a known state
    }
  }
}

void ProgramChangeLog::logAdded(UID id, const std::string& reason,
                                const std::string& submitter) {
  writeEntry("Added", id, reason, submitter);
}

void ProgramChangeLog::logUpdated(UID id, const std::string& reason,
                                  const std::string& submitter) {
  writeEntry("Updated", id, reason, submitter);
}

void ProgramChangeLog::logRemoved(UID id, const std::string& reason) {
  writeEntry("Removed", id, reason, "");
}

void ProgramChangeLog::writeEntry(const std::string& action, UID id,
                                  const std::string& reason,
                                  const std::string& submitter) {
  if (!log_stream.good() || !log_stream.is_open()) {
    return;
  }

  log_stream << getTimestamp() << " | " << action << " | " << id.string()
             << " | " << reason;

  if (!submitter.empty()) {
    log_stream << " | " << submitter;
  }

  log_stream << std::endl;
  log_stream.flush();

  // Check if write succeeded
  if (!log_stream.good()) {
    Log::get().debug("Failed to write to program change log");
  }
}

std::string ProgramChangeLog::getTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
#ifdef _WIN32
  // Windows doesn't have localtime_r, but localtime_s is thread-safe
  struct tm timeinfo;
  localtime_s(&timeinfo, &time_t_now);
  ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
#else
  // POSIX systems use localtime_r for thread safety
  struct tm timeinfo;
  localtime_r(&time_t_now, &timeinfo);
  ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
#endif
  return ss.str();
}
