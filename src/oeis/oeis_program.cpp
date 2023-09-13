#include "oeis/oeis_program.hpp"

#include <set>

#include "lang/analyzer.hpp"
#include "lang/comments.hpp"
#include "lang/parser.hpp"
#include "lang/program_util.hpp"
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
    OeisSequence s(arg);
    result.first = parser.parse(s.getProgramPath());
    result.second = s.id;
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
      auto path = OeisSequence(id).getProgramPath();
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

void updateOperand(Operand &op, int64_t target, int64_t largest_used) {
  if (op.type != Operand::Type::DIRECT) {
    return;
  }
  if (op.value.asInt() == Program::INPUT_CELL) {
    op.value = target;
  } else {
    op.value += largest_used;
  }
}

bool OeisProgram::unfold(Program &p) {
  if (ProgramUtil::hasIndirectOperand(p)) {
    return false;
  }
  // find first seq operation
  int64_t seq_index = -1;
  for (size_t i = 0; i < p.ops.size(); i++) {
    if (p.ops[i].type == Operation::Type::SEQ) {
      seq_index = i;
      break;
    }
  }
  if (seq_index < 0) {
    return false;
  }
  // load and check program to be embedded
  Parser parser;
  int64_t id = p.ops[seq_index].source.value.asInt();
  auto p2 = parser.parse(OeisSequence(id).getProgramPath());
  if (ProgramUtil::hasIndirectOperand(p2)) {
    return false;
  }
  // prepare program for embedding
  // remove nops and comments
  ProgramUtil::removeOps(p2, Operation::Type::NOP);
  for (auto &op : p2.ops) {
    if (op.type != Operation::Type::SEQ) {
      op.comment.clear();
    }
  }
  // find cells that are read and uninitialized
  std::set<Operand> initialized, uninitialized;
  initialized.insert(Operand(Operand::Type::DIRECT, Program::INPUT_CELL));
  for (const auto &op : p2.ops) {
    auto meta = Operation::Metadata::get(op.type);
    if (meta.num_operands > 0 && op.target.type == Operand::Type::DIRECT) {
      if (meta.is_reading_target &&
          initialized.find(op.target) == initialized.end()) {
        uninitialized.insert(op.target);
      }
      if (meta.is_writing_target) {
        initialized.insert(op.target);
      }
    }
    if (meta.num_operands > 1 && op.source.type == Operand::Type::DIRECT) {
      if (initialized.find(op.source) == initialized.end()) {
        uninitialized.insert(op.source);
      }
    }
  }
  // initialize cells that are read and were uninitialized
  for (auto cell : uninitialized) {
    p2.ops.insert(p2.ops.begin(),
                  Operation(Operation::Type::MOV, cell,
                            Operand(Operand::Type::CONSTANT, 0)));
  }
  // shift used operands
  int64_t target = p.ops[seq_index].target.value.asInt();
  int64_t largest_used = ProgramUtil::getLargestDirectMemoryCell(p);
  for (auto &op : p2.ops) {
    updateOperand(op.target, target, largest_used);
    updateOperand(op.source, target, largest_used);
  }
  // delete seq operation
  p.ops.erase(p.ops.begin() + seq_index);
  // embed program
  p.ops.insert(p.ops.begin() + seq_index, p2.ops.begin(), p2.ops.end());
  return true;
}

bool OeisProgram::isTooComplex(const Program &p) {
  int64_t level = 0;
  int64_t numLoops = 0;
  bool hasRootSeq = false;
  for (const auto &op : p.ops) {
    switch (op.type) {
      case Operation::Type::LPB:
        level++;
        numLoops++;
        break;
      case Operation::Type::LPE:
        level--;
        break;
      case Operation::Type::SEQ:
        if (level == 0) {
          hasRootSeq = true;
        }
        break;
      default:
        break;
    }
  }
  return (numLoops > 1) || (numLoops > 0 && hasRootSeq);
}

bool OeisProgram::autoUnfold(Program &p) {
  bool changed = false;
  while (true) {
    // try to unfold
    auto copy = p;
    if (!unfold(copy)) {
      break;
    }
    // abort if unfolded program is too complex
    if (isTooComplex(copy)) {
      break;
    }
    // ok, update program!
    p = copy;
    changed = true;
  }
  return changed;
}

bool updateOperandFold(const Operand &src, const Operand &trg,
                       std::map<int64_t, int64_t> &cell_map) {
  if (src.type != trg.type) {
    return false;
  }
  switch (src.type) {
    case Operand::Type::CONSTANT: {
      if (src.value != trg.value) {
        return false;
      }
      break;
    }
    case Operand::Type::DIRECT: {
      auto s = src.value.asInt();
      auto t = trg.value.asInt();
      auto it = cell_map.find(s);
      if (it == cell_map.end()) {
        cell_map[s] = t;
      } else if (it->second != t) {
        return false;
      }
      break;
    }
    case Operand::Type::INDIRECT: {
      return false;
    }
  }
  return true;
}

bool OeisProgram::fold(Program &main, const Program &sub, size_t subId,
                       std::map<int64_t, int64_t> &cell_map) {
  // find a match
  size_t main_pos = 0, sub_pos = 0;
  while (sub_pos < sub.ops.size() && main_pos < main.ops.size()) {
    bool reset = false;
    if (sub.ops[sub_pos].type != main.ops[main_pos].type) {
      reset = true;
    } else if (!updateOperandFold(sub.ops[sub_pos].target,
                                  main.ops[main_pos].target, cell_map)) {
      reset = true;
    } else if (!updateOperandFold(sub.ops[sub_pos].source,
                                  main.ops[main_pos].source, cell_map)) {
      reset = true;
    }
    if (reset) {
      main_pos = main_pos - sub_pos + 1;
      sub_pos = 0;
      cell_map.clear();
    } else {
      main_pos++;
      sub_pos++;
    }
  }
  // ensure we matched full subprogram
  if (sub_pos < sub.ops.size()) {
    return false;
  }
  // no indirect ops allowed
  if (ProgramUtil::hasIndirectOperand(main) ||
      ProgramUtil::hasIndirectOperand(sub)) {
    return false;
  }
  // apply folding on a copy
  auto folded = main;
  folded.ops[main_pos - 1] = Operation(
      Operation::Type::SEQ,
      Operand(Operand::Type::DIRECT, Number(cell_map.at(Program::INPUT_CELL))),
      Operand(Operand::Type::CONSTANT, Number(subId)));
  folded.ops.erase(folded.ops.begin() + (main_pos - sub.ops.size()),
                   folded.ops.begin() + (main_pos + sub.ops.size() - 1));
  //  check mapping
  /*
  for (auto &e : cell_map) {
    if (e.first == Program::INPUT_CELL) {
    } else {
    }
  }
  */
  return true;
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
          OeisSequence seq(id_str);
          if (isFile(seq.getProgramPath())) {
            if (status == "A" && num_added_ids < max_added_programs) {
              Log::get().debug("Added program for " + seq.id_str());
              ids.insert(seq.id);
              num_added_ids++;
            } else if (status == "M" &&
                       num_modified_ids < max_modified_programs) {
              Log::get().debug("Modified program for " + seq.id_str());
              ids.insert(seq.id);
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
