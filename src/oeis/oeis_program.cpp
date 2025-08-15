#include "oeis/oeis_program.hpp"

#include <set>

#include "lang/analyzer.hpp"
#include "lang/comments.hpp"
#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "lang/subprogram.hpp"
#include "oeis/oeis_sequence.hpp"
#include "sys/file.hpp"
#include "sys/git.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"

std::pair<Program, size_t> OeisProgram::getProgramAndSeqId(
    const std::string &arg) {
  Parser parser;
  std::pair<Program, size_t> result;
  try {
    UID uid(arg);
    result.first = parser.parse(ProgramUtil::getProgramPath(uid.number()));
    result.second = uid.number();
  } catch (...) {
    // not an ID string
    result.first = parser.parse(arg);
    result.second = 0;
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
      auto id = op.source.value.asInt();
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

std::vector<bool> OeisProgram::collectLatestProgramIds(
    size_t max_commits, size_t max_added_programs,
    size_t max_modified_programs) {
  std::vector<bool> latest_program_ids;
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
  std::set<int64_t> ids;
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
          if (isFile(ProgramUtil::getProgramPath(uid.number()))) {
            if (status == "A" && num_added_ids < max_added_programs) {
              Log::get().debug("Added program for " + uid.string());
              ids.insert(uid.number());
              num_added_ids++;
            } else if (status == "M" &&
                       num_modified_ids < max_modified_programs) {
              Log::get().debug("Modified program for " + uid.string());
              ids.insert(uid.number());
              num_modified_ids++;
            }
          }
        } catch (const std::exception &) {
          // ignore because it is not a program of an OEIS sequence
        }
      }
    }
  }
  for (auto id : ids) {
    if (id >= static_cast<int64_t>(latest_program_ids.size())) {
      const size_t new_size =
          std::max<size_t>(id + 1, 2 * latest_program_ids.size());
      latest_program_ids.resize(new_size);
    }
    latest_program_ids[id] = true;
  }
  if (latest_program_ids.empty()) {
    Log::get().warn("Cannot read programs commit history");
  }
  return latest_program_ids;
}
