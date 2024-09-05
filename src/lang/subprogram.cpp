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

bool prepareEmbedding(int64_t id, Program &sub) {
  // load and check program to be embedded
  Parser parser;
  sub = parser.parse(ProgramUtil::getProgramPath(id));
  if (ProgramUtil::hasIndirectOperand(sub)) {
    return false;
  }
  // prepare program for embedding
  // remove nops and comments
  ProgramUtil::removeOps(sub, Operation::Type::NOP);
  for (auto &op : sub.ops) {
    if (op.type != Operation::Type::SEQ) {
      op.comment.clear();
    }
  }
  // find cells that are read and uninitialized
  std::set<int64_t> initialized, uninitialized;
  initialized.insert(Program::INPUT_CELL);
  ProgramUtil::getUsedUninitializedCells(sub, initialized, uninitialized);
  // initialize cells that are read and were uninitialized
  for (auto cell : uninitialized) {
    sub.ops.insert(
        sub.ops.begin(),
        Operation(Operation::Type::MOV, Operand(Operand::Type::CONSTANT, cell),
                  Operand(Operand::Type::CONSTANT, 0)));
  }
  return true;
}

bool Subprogram::unfold(Program &main) {
  if (ProgramUtil::hasIndirectOperand(main)) {
    return false;
  }
  // TODO unfold prg instructions, too
  // find first seq operation
  int64_t seq_index = -1;
  for (size_t i = 0; i < main.ops.size(); i++) {
    if (main.ops[i].type == Operation::Type::SEQ) {
      seq_index = i;
      break;
    }
  }
  if (seq_index < 0) {
    return false;
  }
  auto sub_id = main.ops[seq_index].source.value.asInt();
  Program sub;
  if (!prepareEmbedding(sub_id, sub)) {
    return false;
  }
  // shift used operands
  auto target = main.ops[seq_index].target.value.asInt();
  auto largest_used = ProgramUtil::getLargestDirectMemoryCell(main);
  for (auto &op : sub.ops) {
    updateOperand(op.target, target, largest_used);
    updateOperand(op.source, target, largest_used);
  }
  // delete seq operation
  main.ops.erase(main.ops.begin() + seq_index);
  // embed program
  main.ops.insert(main.ops.begin() + seq_index, sub.ops.begin(), sub.ops.end());
  return true;
}

bool Subprogram::autoUnfold(Program &main) {
  bool changed = false;
  while (true) {
    // try to unfold
    auto copy = main;
    if (!unfold(copy)) {
      break;
    }
    // abort if unfolded program is too complex
    if (shouldFold(copy)) {
      break;
    }
    // ok, update program!
    main = copy;
    changed = true;
  }
  return changed;
}

bool Subprogram::shouldFold(const Program &main) {
  int64_t level = 0;
  int64_t numLoops = 0;
  bool hasRootSeq = false;
  for (const auto &op : main.ops) {
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
