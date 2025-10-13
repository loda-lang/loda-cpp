#include "eval/fold.hpp"

#include "eval/evaluator_par.hpp"
#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "lang/subprogram.hpp"

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

bool prepareEmbedding(UID id, Program &sub, Operation::Type embeddingType) {
  // load and check program to be embedded
  const auto path = ProgramUtil::getProgramPath(id);
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

bool Fold::canUnfold(Operation::Type type) {
  return type == Operation::Type::SEQ || type == Operation::Type::PRG;
}

bool Fold::unfold(Program &main, int64_t pos) {
  if (ProgramUtil::hasIndirectOperand(main)) {
    return false;
  }
  if (pos < 0) {
    // find first operation that can be unfolded
    for (size_t i = 0; i < main.ops.size(); i++) {
      if (canUnfold(main.ops[i].type)) {
        pos = i;
        break;
      }
    }
  }
  if (pos < 0 || static_cast<size_t>(pos) >= main.ops.size() ||
      !canUnfold(main.ops[pos].type)) {
    return false;
  }
  const auto &emb_op = main.ops[pos];
  auto sub_id = emb_op.source.value.asInt();
  auto sub_uid = UID::castFromInt(sub_id);
  Program sub;
  if (!prepareEmbedding(sub_uid, sub, emb_op.type)) {
    return false;
  }
  // shift used operands
  int64_t start = main.ops[pos].target.value.asInt();
  int64_t shared_region_length = 1;
  if (emb_op.type == Operation::Type::PRG) {
    shared_region_length = std::max<int64_t>(sub.getDirective("inputs"),
                                             sub.getDirective("outputs"));
  }
  int64_t largest_used = 0;
  if (!ProgramUtil::getUsedMemoryCells(main, nullptr, largest_used, -1)) {
    return false;
  }
  for (auto &op : sub.ops) {
    updateOperand(op.target, start, shared_region_length, largest_used);
    updateOperand(op.source, start, shared_region_length, largest_used);
  }
  // delete old operation
  main.ops.erase(main.ops.begin() + pos);
  // embed program
  main.ops.insert(main.ops.begin() + pos, sub.ops.begin(), sub.ops.end());
  return true;
}

bool Fold::autoUnfold(Program &main) {
  bool changed = false;
  while (true) {
    auto copy = main;
    bool unfolded = false;
    for (size_t i = 0; i < copy.ops.size(); i++) {
      // try to unfold
      if (!unfold(copy, i)) {
        continue;
      }
      // revert if unfolded program is too complex
      if (shouldFold(copy)) {
        copy = main;
      } else {
        unfolded = true;
        break;
      }
    }
    // update main program if unfolding was successful
    if (unfolded) {
      main = copy;
      changed = true;
    } else {
      break;
    }
  }
  return changed;
}

bool Fold::shouldFold(const Program &main) {
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

bool Fold::fold(Program &main, Program sub, size_t subId,
                std::map<int64_t, int64_t> &cellMap, int64_t maxMemory) {
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
  // search for subprogram in main program
  auto mainPos = Subprogram::search(main, sub, cellMap);
  if (mainPos < 0) {
    return false;
  }
  // get used memory cells
  std::unordered_set<int64_t> used_sub_cells;
  int64_t tmp_larged_used;
  if (!ProgramUtil::getUsedMemoryCells(sub, &used_sub_cells, tmp_larged_used,
                                       maxMemory)) {
    return false;
  }
  int64_t largest_used_main = 0;
  if (!ProgramUtil::getUsedMemoryCells(main, nullptr, largest_used_main,
                                       maxMemory)) {
    return false;
  }
  // initialize partial evaluator for main program
  Settings settings;
  PartialEvaluator eval(settings);
  eval.initZeros(Program::INPUT_CELL + 1, largest_used_main);
  // check usage of sub cells in main program
  for (size_t i = 0; i < main.ops.size(); i++) {
    const auto &op = main.ops[i];
    for (auto cell : used_sub_cells) {
      if (cell == Program::OUTPUT_CELL) {
        continue;
      }
      auto t = cellMap.find(cell);
      if (t == cellMap.end()) {
        continue;
      }
      const int64_t mapped = t->second;
      // check if main program is reading cells that are used by subprogram
      const int64_t ii = static_cast<int64_t>(i);
      const int64_t end = mainPos + sub.ops.size();
      if ((ii < mainPos || ii >= end) &&
          ProgramUtil::isReadingCell(op, mapped)) {
        return false;
      }
      // ensure that cells used by subprogram are initialized with zero before
      // subprogram is executed
      if (ii == mainPos && !eval.checkValue(mapped, 0)) {
        return false;
      }
    }
    eval.doPartialEval(main, i);
  }
  // perform folding on main program
  auto mappedInput = cellMap.at(Program::INPUT_CELL);
  main.ops.erase(main.ops.begin() + mainPos,
                 main.ops.begin() + mainPos + sub.ops.size());
  main.ops.insert(main.ops.begin() + mainPos,
                  Operation(Operation::Type::SEQ,
                            Operand(Operand::Type::DIRECT, Number(mappedInput)),
                            Operand(Operand::Type::CONSTANT, Number(subId))));
  return true;
}
