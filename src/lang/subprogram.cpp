#include "lang/subprogram.hpp"

#include "lang/parser.hpp"
#include "lang/program_util.hpp"

size_t Subprogram::replaceAllExact(Program &main, const Program &search,
                                   const Program &replace) {
  if (main.ops.empty() || search.ops.empty() ||
      search.ops.size() > main.ops.size()) {
    return 0;
  }
  size_t count = 0;
  const size_t max_start = main.ops.size() - search.ops.size();
  for (size_t i = 0; i < max_start; i++) {
    bool matches = true;
    for (size_t j = 0; j < search.ops.size(); j++) {
      if (main.ops[i + j] != search.ops[j]) {
        matches = false;
        break;
      }
    }
    if (matches) {
      main.ops.erase(main.ops.begin() + i,
                     main.ops.begin() + i + search.ops.size());
      main.ops.insert(main.ops.begin() + i, replace.ops.begin(),
                      replace.ops.end());
      i += replace.ops.size() - 1;
      count++;
    }
  }
  return count;
}

bool updateOperand(const Operand &src, const Operand &trg,
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

int64_t Subprogram::search(const Program &main, const Program &sub,
                           std::map<int64_t, int64_t> &cell_map) {
  // find a match
  size_t main_pos = 0, sub_pos = 0;
  while (sub_pos < sub.ops.size() && main_pos < main.ops.size()) {
    bool reset = false;
    if (sub.ops[sub_pos].type != main.ops[main_pos].type) {
      reset = true;
    } else if (!updateOperand(sub.ops[sub_pos].target,
                              main.ops[main_pos].target, cell_map)) {
      reset = true;
    } else if (!updateOperand(sub.ops[sub_pos].source,
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
    return -1;
  }
  return main_pos - sub_pos;
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

bool Subprogram::unfold(Program &p) {
  if (ProgramUtil::hasIndirectOperand(p)) {
    return false;
  }
  // TODO unfold prg instructions, too
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
  auto p2 = parser.parse(ProgramUtil::getProgramPath(id));
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
  std::set<int64_t> initialized, uninitialized;
  initialized.insert(Program::INPUT_CELL);
  ProgramUtil::getUsedUninitializedCells(p2, initialized, uninitialized);

  // initialize cells that are read and were uninitialized
  for (auto cell : uninitialized) {
    p2.ops.insert(
        p2.ops.begin(),
        Operation(Operation::Type::MOV, Operand(Operand::Type::CONSTANT, cell),
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

bool Subprogram::autoUnfold(Program &p) {
  bool changed = false;
  while (true) {
    // try to unfold
    auto copy = p;
    if (!unfold(copy)) {
      break;
    }
    // abort if unfolded program is too complex
    if (shouldFold(copy)) {
      break;
    }
    // ok, update program!
    p = copy;
    changed = true;
  }
  return changed;
}

bool Subprogram::shouldFold(const Program &p) {
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

bool Subprogram::fold(Program &main, Program sub, size_t subId,
                      std::map<int64_t, int64_t> &cell_map) {
  // prepare and check subprogram
  ProgramUtil::removeOps(sub, Operation::Type::NOP);
  if (sub.ops.empty()) {
    return false;
  }
  int64_t output_cell = Program::OUTPUT_CELL;
  auto last = sub.ops.back();
  if (sub.ops.size() > 1 && last.type == Operation::Type::MOV &&
      last.target == Operand(Operand::Type::DIRECT, Program::OUTPUT_CELL) &&
      last.source.type == Operand::Type::DIRECT) {
    output_cell = last.source.value.asInt();
    sub.ops.pop_back();
  }
  auto main_pos = Subprogram::search(main, sub, cell_map);
  // no indirect ops allowed
  if (ProgramUtil::hasIndirectOperand(main) ||
      ProgramUtil::hasIndirectOperand(sub)) {
    return false;
  }
  // perform folding on main program
  const Number mapped_input(cell_map.at(Program::INPUT_CELL));
  const Number mapped_output(cell_map.at(output_cell));
  main.ops.erase(main.ops.begin() + main_pos,
                 main.ops.begin() + main_pos + sub.ops.size());
  main.ops.insert(main.ops.begin() + main_pos,
                  Operation(Operation::Type::SEQ,
                            Operand(Operand::Type::DIRECT, mapped_input),
                            Operand(Operand::Type::CONSTANT, Number(subId))));
  main.ops.insert(main.ops.begin() + main_pos + 1,
                  Operation(Operation::Type::MOV,
                            Operand(Operand::Type::DIRECT, mapped_output),
                            Operand(Operand::Type::DIRECT, mapped_input)));
  return true;
}
