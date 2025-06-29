#include "lang/program_cache.hpp"

#include <exception>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include "lang/parser.hpp"
#include "lang/program_util.hpp"

const Program& ProgramCache::getProgram(int64_t id) {
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

std::string ProgramCache::getProgramPath(int64_t id) {
  if (id < 0) {
    return ProgramUtil::getProgramPath(-id, "prg", "P");
  } else {
    return ProgramUtil::getProgramPath(id);
  }
}

std::unordered_map<int64_t, Program> ProgramCache::collect(int64_t id) {
  std::unordered_map<int64_t, Program> result;
  std::unordered_set<int64_t> visiting;
  std::vector<int64_t> stack;
  stack.push_back(id);
  while (!stack.empty()) {
    int64_t cur_id = stack.back();
    stack.pop_back();
    if (result.count(cur_id)) {
      continue;  // already collected
    }
    if (visiting.count(cur_id)) {
      throw std::runtime_error(
          "Recursion detected in program dependencies at id: " +
          std::to_string(cur_id));
    }
    visiting.insert(cur_id);
    const Program& prog = getProgram(cur_id);
    result[cur_id] = prog;
    for (const auto& op : prog.ops) {
      if (op.type == Operation::Type::SEQ &&
          op.source.type == Operand::Type::CONSTANT) {
        int64_t dep_id = op.source.value.asInt();
        if (!result.count(dep_id)) {
          stack.push_back(dep_id);
        }
      }
    }
    visiting.erase(cur_id);
  }
  return result;
}

int64_t ProgramCache::getOffset(int64_t id) {
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

void ProgramCache::clear() {
  programs.clear();
  offsets.clear();
  missing.clear();
}
