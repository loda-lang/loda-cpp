#pragma once

#include <map>

#include "base/uid.hpp"
#include "lang/program.hpp"

class OeisProgram {
 public:
  static std::pair<Program, UID> getProgramAndSeqId(
      const std::string& id_or_path);

  static size_t getTransitiveProgramHash(const Program& p);

  static size_t getNumCheckTerms(bool full_check);

  static size_t getNumRequiredTerms(const Program& p);

  static size_t getNumMinimizationTerms(const Program& p);

  static UIDSet collectLatestProgramIds(size_t max_commits,
                                        size_t max_added_programs,
                                        size_t max_modified_programs);
};
