#pragma once

#include <map>
#include <string>

#include "base/uid.hpp"
#include "sys/util.hpp"

class InvalidMatches {
 public:
  InvalidMatches();
  void load();
  void insert(UID id);
  static void deleteFile();
  bool hasTooMany(UID id) const;

 private:
  std::map<UID, int64_t> invalid_matches;
  AdaptiveScheduler scheduler;
};
