#include "lang/program_cache.hpp"

#include <exception>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include "lang/parser.hpp"
#include "lang/program_util.hpp"

const Program& ProgramCache::getProgram(UID id) {
  if (missing.find(id) != missing.end()) {
    throw std::runtime_error("Program not found: " + getProgramPath(id));
  }
  if (programs.find(id) == programs.end()) {
    try {
      Parser parser;
      programs[id] = parser.parse(getProgramPath(id));
    } catch (...) {
      missing.insert(id);
      std::rethrow_exception(std::current_exception());
    }
  }
  return programs[id];
}

std::string ProgramCache::getProgramPath(UID id) {
  switch (id.domain()) {
    case 'A':
      return ProgramUtil::getProgramPath(id);
    case 'P':
      return ProgramUtil::getProgramPath(id, "prg", "P");
    default:
      throw std::runtime_error("Unknown program path for " + id.string());
  }
}

std::unordered_map<UID, Program> ProgramCache::collect(UID id) {
  std::unordered_map<UID, Program> result;
  std::unordered_set<UID> visiting;
  std::vector<UID> stack;
  stack.push_back(id);
  while (!stack.empty()) {
    auto cur_id = stack.back();
    stack.pop_back();
    if (result.count(cur_id)) {
      continue;  // already collected
    }
    if (visiting.count(cur_id)) {
      throw std::runtime_error("Recursion detected in program dependencies: " +
                               cur_id.string());
    }
    visiting.insert(cur_id);
    const Program& prog = getProgram(cur_id);
    result[cur_id] = prog;
    for (const auto& op : prog.ops) {
      if ((op.type == Operation::Type::SEQ ||
           op.type == Operation::Type::PRG) &&
          op.source.type == Operand::Type::CONSTANT) {
        auto dep_id = UID::castFromInt(op.source.value.asInt());
        if (!result.count(dep_id)) {
          stack.push_back(dep_id);
        }
      }
    }
    visiting.erase(cur_id);
  }
  return result;
}

int64_t ProgramCache::getOffset(UID id) {
  if (offsets.find(id) == offsets.end()) {
    try {
      const Program& prog = getProgram(id);
      offsets[id] = ProgramUtil::getOffset(prog);
    } catch (...) {
      missing.insert(id);
      std::rethrow_exception(std::current_exception());
    }
  }
  return offsets[id];
}

bool ProgramCache::shouldCheckOffset(UID id) const {
  return skip_check_offsets.find(id) == skip_check_offsets.end();
}

void ProgramCache::setCheckOffset(UID id, bool check) {
  if (check) {
    skip_check_offsets.erase(id);  // check enabled per default
  } else {
    skip_check_offsets.insert(id);
  }
}

int64_t ProgramCache::getOverhead(UID id) const {
  const auto it = overheads.find(id);
  return it != overheads.end() ? it->second : 0;
}

void ProgramCache::setOverhead(UID id, int64_t overhead) {
  overheads[id] = overhead;
}

void ProgramCache::insert(UID id, const Program& p) {
  programs[id] = p;
  missing.erase(id);
  offsets.erase(id);
}

void ProgramCache::clear() {
  programs.clear();
  offsets.clear();
  overheads.clear();
  missing.clear();
  skip_check_offsets.clear();
}
