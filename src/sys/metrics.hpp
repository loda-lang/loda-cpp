#pragma once

#include <map>
#include <string>
#include <vector>

class Metrics {
 public:
  struct Entry {
    std::string field;
    std::map<std::string, std::string> labels;
    double value;
  };

  Metrics();

  static Metrics& get();

  void write(const std::vector<Entry>& entries) const;

  const int64_t publish_interval;

 private:
  std::string host;
  std::string auth;
  int64_t tmp_file_id;
  mutable bool notified;
};
