#pragma once

#include <unordered_map>
#include <unordered_set>

#include "base/uid.hpp"
#include "lang/program.hpp"

class ProgramCache {
 public:
  const Program &getProgram(UID id);

  int64_t getOffset(UID id);

  bool shouldCheckOffset(UID id) const;

  void setCheckOffset(UID id, bool check);

  int64_t getOverhead(UID id) const;

  void setOverhead(UID id, int64_t overhead);

  std::unordered_map<UID, Program> collect(UID id);

  void insert(UID id, const Program &p);

  void clear();

 private:
  std::unordered_map<UID, Program> programs;
  std::unordered_map<UID, int64_t> offsets;
  std::unordered_map<UID, int64_t> overheads;
  std::unordered_set<UID> missing;
  std::unordered_set<UID> skip_check_offsets;
};
