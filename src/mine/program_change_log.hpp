#pragma once

#include <fstream>
#include <string>

class UID;  // Forward declaration

class ProgramChangeLog {
 public:
  ProgramChangeLog();

  void logAdded(UID id, const std::string& reason,
                const std::string& submitter);
  void logUpdated(UID id, const std::string& reason,
                  const std::string& submitter);
  void logRemoved(UID id, const std::string& reason);

 private:
  void writeEntry(const std::string& action, UID id, const std::string& reason,
                  const std::string& submitter);

  std::string getTimestamp();

  std::string log_file_path;
  std::ofstream log_stream;
};
