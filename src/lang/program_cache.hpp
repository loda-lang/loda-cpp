#pragma once

#include <unordered_map>
#include <unordered_set>

#include "lang/program.hpp"

class ProgramCache {
 public:
  const Program &getProgram(int64_t id);

  int64_t getOffset(int64_t id);

  int64_t getOverhead(int64_t id) const;

  void setOverhead(int64_t id, int64_t overhead);

  std::unordered_map<int64_t, Program> collect(int64_t id);

  static std::string getProgramPath(int64_t id);

  void insert(int64_t id, const Program &p);

  void clear();

 private:
  std::unordered_map<int64_t, Program> programs;
  std::unordered_map<int64_t, int64_t> offsets;
  std::unordered_map<int64_t, int64_t> overheads;
  std::unordered_set<int64_t> missing;
};
