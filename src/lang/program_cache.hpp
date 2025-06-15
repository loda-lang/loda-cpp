#pragma once

#include <unordered_map>
#include <unordered_set>

#include "lang/program.hpp"

class ProgramCache {
 public:
  const Program &get(int64_t id);

  std::unordered_map<int64_t, Program> collect(int64_t id);

  static std::string getProgramPath(int64_t id);

  void clear();

 private:
  std::unordered_map<int64_t, Program> cache;
  std::unordered_set<int64_t> missing;
};
