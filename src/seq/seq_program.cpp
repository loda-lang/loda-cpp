#include "seq/seq_program.hpp"

#include <iostream>
#include <set>

#include "lang/analyzer.hpp"
#include "lang/comments.hpp"
#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "lang/subprogram.hpp"
#include "seq/managed_seq.hpp"
#include "sys/file.hpp"
#include "sys/git.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"

std::pair<Program, UID> SequenceProgram::getProgramAndSeqId(
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

size_t SequenceProgram::getTransitiveProgramHash(const Program &program) {
  std::set<Program> collected;
  collectPrograms(program, collected);
  size_t h = 0;
  for (auto &p : collected) {
    h += ProgramUtil::hash(p);
  }
  return h;
}

size_t SequenceProgram::getNumCheckTerms(bool full_check) {
  return full_check ? SequenceUtil::FULL_SEQ_LENGTH
                    : SequenceUtil::EXTENDED_SEQ_LENGTH;
}

size_t SequenceProgram::getNumRequiredTerms(const Program &p) {
  return SequenceUtil::DEFAULT_SEQ_LENGTH;
}

size_t SequenceProgram::getNumMinimizationTerms(const Program &p) {
  return getNumRequiredTerms(p) * 2;  // magic number
}

UIDSet SequenceProgram::collectLatestProgramIds(size_t max_commits,
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

void SequenceProgram::commitAddedPrograms(size_t min_commit_count) {
  auto progs_dir = Setup::getProgramsHome();
  auto status_entries = Git::status(progs_dir);

  std::vector<std::string> files_to_add;
  for (const auto &entry : status_entries) {
    const std::string &status = entry.first;
    const std::string &file = entry.second;
    if (status == "??" && file.find("oeis/") == 0) {
      files_to_add.push_back(file);
    }
  }

  for (const auto &file : files_to_add) {
    if (!Git::add(progs_dir, file)) {
      Log::get().warn("Failed to add file: " + file);
    }
  }

  if (files_to_add.size() >= min_commit_count) {
    std::string msg =
        "added " + std::to_string(files_to_add.size()) + " programs";
    if (!Git::commit(progs_dir, msg)) {
      Log::get().warn("Failed to commit added programs");
    }
  }

  if (!Git::push(progs_dir)) {
    Log::get().warn("Failed to push changes");
  }
}

void SequenceProgram::commitUpdateAndDeletedPrograms(
    Stats *stats, const std::unordered_set<UID> *full_check_list) {
  auto progs_dir = Setup::getProgramsHome();
  auto status_entries = Git::status(progs_dir);

  std::vector<std::string> files_to_update;
  std::vector<std::string> files_to_delete;

  for (const auto &entry : status_entries) {
    const std::string &status = entry.first;
    const std::string &file = entry.second;
    if (file.find("oeis/") != 0) {
      continue;
    }
    if (status == " M") {
      files_to_update.push_back(file);
    } else if (status == " D") {
      files_to_delete.push_back(file);
    }
  }

  size_t num_updated = 0;
  size_t num_deleted = 0;

  // Handle updated files
  Parser parser;
  for (const auto &file : files_to_update) {
    // Load new version
    Program new_program;
    try {
      new_program = parser.parse(file);
    } catch (const std::exception &e) {
      std::cerr << "Failed to parse new version: " << file << std::endl;
    }

    // Load old version using Git helper
    Program old_program;
    try {
      std::string tmp_old = Git::extractHeadVersion(progs_dir, file);
      if (!tmp_old.empty()) {
        old_program = parser.parse(tmp_old);
        std::remove(tmp_old.c_str());
      } else {
        std::cerr << "Failed to load old version from git for: " << file
                  << std::endl;
      }
    } catch (const std::exception &e) {
      std::cerr << "Failed to parse old version: " << file << std::endl;
    }

    // Check if already staged
    if (Git::git(progs_dir, "diff -U1000 --exit-code -- \"" + file + "\"",
                 false)) {
      std::cout << "Already staged: " << file << std::endl;
      num_updated++;
      continue;
    }
    std::cout << "\n";

    std::string fname = file.substr(file.find_last_of("/\\") + 1);
    std::string anumber = fname.substr(0, fname.find('.'));

    // Usage info and warnings
    int64_t usage = 0;
    if (stats) {
      UID uid(anumber);
      usage = stats->getNumUsages(uid);
      if (usage > 0) {
        std::cout << usage << " other programs using this program.\n\n";
      }
      if (usage >= 100) {
        std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        std::cout << "!!!   HIGH USAGE WARNING   !!!\n";
        std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
      }
    }

    // Full check info
    if (full_check_list) {
      UID uid(anumber);
      if (full_check_list->count(uid) > 0) {
        std::cout << "Full check enabled.\n\n";
      }
    }

    // You can now compare old_program and new_program here if needed

    std::cout << "Update " << anumber << "? (Y)es, (n)o, (r)evert: ";
    std::string answer;
    std::getline(std::cin, answer);

    if (answer.empty() || answer == "y" || answer == "Y") {
      Git::add(progs_dir, file);
      num_updated++;
    } else if (answer == "r" || answer == "R") {
      Git::git(progs_dir, "checkout -- \"" + file + "\"");
      std::cout << "Reverted: " << file << std::endl;
    } else {
      Git::git(progs_dir, "checkout -- \"" + file + "\"");
      std::cout << "Restored: " << file << std::endl;
    }
  }

  // Handle deleted files
  for (const auto &file : files_to_delete) {
    std::string fname = file.substr(file.find_last_of("/\\") + 1);
    std::string anumber = fname.substr(0, fname.find('.'));

    std::cout << "\nDelete " << anumber << "? (Y)es, (n)o: ";
    std::string answer;
    std::getline(std::cin, answer);

    if (answer.empty() || answer == "y" || answer == "Y") {
      if (Git::add(progs_dir, file)) {
        num_deleted++;
        std::cout << "Staged for deletion: " << file << std::endl;
      } else {
        Log::get().warn("Failed to stage deleted file: " + file);
      }
    } else {
      Git::git(progs_dir, "checkout -- \"" + file + "\"");
      std::cout << "Restored: " << file << std::endl;
    }
  }

  // Single commit at the end
  if ((num_updated + num_deleted) > 0) {
    std::cout << "Commit " << num_updated << " updated and " << num_deleted
              << " deleted programs? (Y/n): ";
    std::string answer;
    std::getline(std::cin, answer);
    if (answer.empty() || answer == "y" || answer == "Y") {
      std::string msg = "updated " + std::to_string(num_updated) +
                        " and deleted " + std::to_string(num_deleted) +
                        " programs";
      if (!Git::commit(progs_dir, msg)) {
        Log::get().warn("Failed to commit changes");
      }
      if (!Git::push(progs_dir)) {
        Log::get().warn("Failed to push changes");
      }
    }
  }
}
