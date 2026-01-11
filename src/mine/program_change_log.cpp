#include "mine/program_change_log.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>

#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"

ProgramChangeLog::ProgramChangeLog() {
  // Only create the log file in server mode
  if (Setup::getMiningMode() == MINING_MODE_SERVER) {
    log_file_path = Setup::getDebugHome() + "programs_changelog.txt";
    log_stream.open(log_file_path, std::ios::app);
    if (!log_stream.good()) {
      Log::get().warn("Failed to open program change log file: " +
                      log_file_path);
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
  if (!log_stream.good()) {
    return;
  }

  log_stream << getTimestamp() << " | " << action << " | " << id.string()
             << " | " << reason;

  if (!submitter.empty()) {
    log_stream << " | " << submitter;
  }

  log_stream << std::endl;
  log_stream.flush();
}

std::string ProgramChangeLog::getTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}
