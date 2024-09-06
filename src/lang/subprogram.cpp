#include "lang/subprogram.hpp"

#include <stdexcept>

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

void updateOperand(Operand &op, int64_t start, int64_t shared_region_length,
                   int64_t largest_used) {
  if (op.type != Operand::Type::DIRECT) {
    return;
  }
  auto v = op.value.asInt();
  if (v < shared_region_length) {
    op.value += start;
  } else {
    op.value += largest_used;
  }
}

bool prepareEmbedding(int64_t id, Program &sub, Operation::Type embeddingType) {
  // load and check program to be embedded
  std::string path;
  if (embeddingType == Operation::Type::SEQ) {
    path = ProgramUtil::getProgramPath(id);
  } else if (embeddingType == Operation::Type::PRG) {
    path = ProgramUtil::getProgramPath(id, "prg", "P");
  } else {
    throw std::runtime_error("Unsupported embedding type");
  }
  Parser parser;
  sub = parser.parse(path);
  if (ProgramUtil::hasIndirectOperand(sub)) {
    return false;
  }
  // prepare program for embedding: remove nops and comments
  ProgramUtil::removeOps(sub, Operation::Type::NOP);
  for (auto &op : sub.ops) {
    if (op.type != Operation::Type::SEQ) {
      op.comment.clear();
    }
  }
  // find cells that are read and uninitialized
  std::set<int64_t> initialized, uninitialized;
  if (embeddingType == Operation::Type::SEQ) {
    initialized.insert(Program::INPUT_CELL);
  } else if (embeddingType == Operation::Type::PRG) {
    auto numInputs = sub.getDirective("inputs");
    for (int64_t i = 0; i < numInputs; i++) {
      initialized.insert(i);
    }
  } else {
    throw std::runtime_error("Unsupported embedding type");
  }
  ProgramUtil::getUsedUninitializedCells(sub, initialized, uninitialized);
  // initialize cells that are read and were uninitialized
  for (auto cell : uninitialized) {
    sub.ops.insert(
        sub.ops.begin(),
        Operation(Operation::Type::MOV, Operand(Operand::Type::DIRECT, cell),
                  Operand(Operand::Type::CONSTANT, 0)));
  }
  return true;
}

bool Subprogram::unfold(Program &main) {
  if (ProgramUtil::hasIndirectOperand(main)) {
    return false;
  }
  // find first seq or prg operation
  int64_t pos = -1;
  for (size_t i = 0; i < main.ops.size(); i++) {
    if (main.ops[i].type == Operation::Type::SEQ ||
        main.ops[i].type == Operation::Type::PRG) {
      pos = i;
      break;
    }
  }
  if (pos < 0) {
    return false;
  }
  const auto &emb_op = main.ops[pos];
  auto sub_id = emb_op.source.value.asInt();
  Program sub;
  if (!prepareEmbedding(sub_id, sub, emb_op.type)) {
    return false;
  }
  // shift used operands
  int64_t start = main.ops[pos].target.value.asInt();
  int64_t shared_region_length = 1;
  if (emb_op.type == Operation::Type::PRG) {
    shared_region_length = std::max<int64_t>(sub.getDirective("inputs"),
                                             sub.getDirective("outputs"));
  }
  int64_t largest_used = ProgramUtil::getLargestDirectMemoryCell(main);
  for (auto &op : sub.ops) {
    updateOperand(op.target, start, shared_region_length, largest_used);
    updateOperand(op.source, start, shared_region_length, largest_used);
  }
  // delete seq operation
  main.ops.erase(main.ops.begin() + pos);
  // embed program
  main.ops.insert(main.ops.begin() + pos, sub.ops.begin(), sub.ops.end());
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
  bool hasRootRef = false;
  for (const auto &op : main.ops) {
    switch (op.type) {
      case Operation::Type::LPB:
        level++;
        numLoops++;
        break;
      case Operation::Type::LPE:
        level--;
        break;
      case Operation::Type::PRG:
      case Operation::Type::SEQ:
        if (level == 0) {
          hasRootRef = true;
        }
        break;
      default:
        break;
    }
  }
  return (numLoops > 1) || (numLoops > 0 && hasRootRef);
}

bool Subprogram::fold(Program &main, Program sub, size_t subId,
                      std::map<int64_t, int64_t> &cellMap) {
  // no indirect operands  allowed
  if (ProgramUtil::hasIndirectOperand(main) ||
      ProgramUtil::hasIndirectOperand(sub)) {
    return false;
  }
  // prepare and check subprogram
  ProgramUtil::removeOps(sub, Operation::Type::NOP);
  if (sub.ops.empty()) {
    return false;
  }
  int64_t outputCell = Program::OUTPUT_CELL;
  auto last = sub.ops.back();
  if (sub.ops.size() > 1 && last.type == Operation::Type::MOV &&
      last.target == Operand(Operand::Type::DIRECT, Program::OUTPUT_CELL) &&
      last.source.type == Operand::Type::DIRECT) {
    outputCell = last.source.value.asInt();
    sub.ops.pop_back();
  }
  auto mainPos = Subprogram::search(main, sub, cellMap);
  if (mainPos < 0) {
    return false;
  }
  // perform folding on main program
  const Number mappedInput(cellMap.at(Program::INPUT_CELL));
  const Number mappedOutput(cellMap.at(outputCell));
  main.ops.erase(main.ops.begin() + mainPos,
                 main.ops.begin() + mainPos + sub.ops.size());
  main.ops.insert(main.ops.begin() + mainPos,
                  Operation(Operation::Type::SEQ,
                            Operand(Operand::Type::DIRECT, mappedInput),
                            Operand(Operand::Type::CONSTANT, Number(subId))));
  if (mappedOutput != mappedInput) {
    main.ops.insert(main.ops.begin() + mainPos + 1,
                    Operation(Operation::Type::MOV,
                              Operand(Operand::Type::DIRECT, mappedOutput),
                              Operand(Operand::Type::DIRECT, mappedInput)));
  }
  return true;
}
