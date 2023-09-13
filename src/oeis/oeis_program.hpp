#pragma once

#include <map>

#include "lang/program.hpp"

class OeisProgram {
 public:
  static std::pair<Program, size_t> getProgramAndSeqId(const std::string& arg);

  static size_t getTransitiveProgramHash(const Program& p);

  static bool isTooComplex(const Program& p);

  static size_t getNumCheckTerms(bool full_check);

  static size_t getNumRequiredTerms(const Program& p);

  static bool unfold(Program& p);

  static bool autoUnfold(Program& p);

  static bool fold(Program& main, const Program& sub, size_t subId,
                   std::map<int64_t, int64_t>& cell_map);

  static std::vector<bool> collectLatestProgramIds(
      size_t max_commits, size_t max_added_programs,
      size_t max_modified_programs);
};
