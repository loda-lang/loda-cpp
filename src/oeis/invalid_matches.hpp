#pragma once

#include <map>
#include <string>

#include "sys/util.hpp"

class InvalidMatches {
 public:
  void load();
  void insert(size_t id);
  static void deleteFile();
  bool hasTooMany(size_t id) const;

 private:
  std::map<size_t, int64_t> invalid_matches;
  AdaptiveScheduler scheduler{1800};
};
