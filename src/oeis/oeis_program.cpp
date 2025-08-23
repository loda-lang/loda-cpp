#include "oeis/oeis_program.hpp"

#include <set>

#include "lang/analyzer.hpp"
#include "lang/comments.hpp"
#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "lang/subprogram.hpp"
#include "seq/sequence.hpp"
#include "sys/file.hpp"
#include "sys/git.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"

std::pair<Program, UID> OeisProgram::getProgramAndSeqId(
    const std::string &id_or_path) {
  Parser parser;
  std::pair<Program, UID> result;
  try {
    UID uid(id_or_path);
    result.first = parser.parse(ProgramUtil::getProgramPath(uid));
    result.second = uid;
  } catch (...) {
    // not an ID string
    result.first = parser.parse(id_or_path);
    result.second = UID('T', 0);
  }
  return result;
}

void collectPrograms(const Program &p, std::set<Program> &collected) {
  if (collected.find(p) != collected.end()) {
    return;
  }
  collected.insert(p);
  for (auto &op : p.ops) {
    if (op.type == Operation::Type::SEQ &&
        op.source.type == Operand::Type::CONSTANT) {
      auto id = UID::castFromInt(op.source.value.asInt());
      auto path = ProgramUtil::getProgramPath(id);
      try {
        Parser parser;
        auto p2 = parser.parse(path);
        collectPrograms(p2, collected);
      } catch (const std::exception &) {
        Log::get().warn("Referenced program not found: " + path);
      }
    }
  }
}

size_t OeisProgram::getTransitiveProgramHash(const Program &program) {
  std::set<Program> collected;
  collectPrograms(program, collected);
  size_t h = 0;
  for (auto &p : collected) {
    h += ProgramUtil::hash(p);
  }
  return h;
}

size_t OeisProgram::getNumCheckTerms(bool full_check) {
  return full_check ? OeisSequence::FULL_SEQ_LENGTH
                    : OeisSequence::EXTENDED_SEQ_LENGTH;
}

size_t OeisProgram::getNumRequiredTerms(const Program &p) {
  return OeisSequence::DEFAULT_SEQ_LENGTH;
  // return Analyzer::hasExponentialComplexity(p)
  //            ? OeisSequence::MIN_NUM_EXP_TERMS
  //            : OeisSequence::DEFAULT_SEQ_LENGTH;
}

size_t OeisProgram::getNumMinimizationTerms(const Program &p) {
  return getNumRequiredTerms(p) * 2;  // magic number
}

UIDSet OeisProgram::collectLatestProgramIds(size_t max_commits,
                                            size_t max_added_programs,
                                            size_t max_modified_programs) {
  UIDSet latest_program_ids;
  auto progs_dir = Setup::getProgramsHome();
  if (!isDir(progs_dir + ".git")) {
    Log::get().warn(
        "Cannot read commit history because the .git folder was not found");
    return latest_program_ids;
  }
  auto commits = Git::log(progs_dir, max_commits);
  if (commits.empty()) {
    Log::get().warn("Cannot read programs commit history");
    return latest_program_ids;
  }
  size_t num_added_ids = 0, num_modified_ids = 0;
  for (const auto &commit : commits) {
    if (num_added_ids >= max_added_programs &&
        num_modified_ids >= max_modified_programs) {
      break;
    }
    auto changes = Git::diffTree(progs_dir, commit);
    for (const auto &c : changes) {
      const auto &status = c.first;
      const auto &path = c.second;
      if (path.size() >= 11 && path.substr(path.size() - 4) == ".asm") {
        auto id_str = path.substr(path.size() - 11, 7);
        try {
          UID uid(id_str);
          if (isFile(ProgramUtil::getProgramPath(uid))) {
            if (status == "A" && num_added_ids < max_added_programs) {
              Log::get().debug("Added program for " + uid.string());
              latest_program_ids.insert(uid);
              num_added_ids++;
            } else if (status == "M" &&
                       num_modified_ids < max_modified_programs) {
              Log::get().debug("Modified program for " + uid.string());
              latest_program_ids.insert(uid);
              num_modified_ids++;
            }
          }
        } catch (const std::exception &) {
          // ignore because it is not a program of an OEIS sequence
        }
      }
    }
  }
  if (latest_program_ids.empty()) {
    Log::get().warn("Cannot read programs commit history");
  }
  return latest_program_ids;
}
