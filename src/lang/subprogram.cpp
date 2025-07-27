#include "lang/subprogram.hpp"

#include <stdexcept>

#include "eval/evaluator_par.hpp"
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

bool Subprogram::canUnfold(Operation::Type type) {
  return type == Operation::Type::SEQ || type == Operation::Type::PRG;
}

bool Subprogram::unfold(Program &main, int64_t pos) {
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
  // delete old operation
  main.ops.erase(main.ops.begin() + pos);
  // embed program
  main.ops.insert(main.ops.begin() + pos, sub.ops.begin(), sub.ops.end());
  return true;
}

bool Subprogram::autoUnfold(Program &main) {
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
  if (!ProgramUtil::getUsedMemoryCells(sub, used_sub_cells, tmp_larged_used,
                                       maxMemory)) {
    return false;
  }
  int64_t largest_used_main = ProgramUtil::getLargestDirectMemoryCell(main);
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

// Helper class for tracking cell usage in a subprogram
class CellTracker {
 public:
  int64_t input_cell = -1;
  int64_t output_cell = -1;
  int64_t open_loops = 0;
  std::set<int64_t> written_cells;

  bool read(int64_t cell, bool after) {
    if (after) {
      if (written_cells.find(cell) != written_cells.end()) {
        if (output_cell == -1) {
          output_cell = cell;
        } else {
          return false;  // multiple output cells found
        }
      }
    } else {
      if (input_cell == -1) {
        input_cell = cell;
      } else if (cell != input_cell &&
                 written_cells.find(cell) == written_cells.end()) {
        return false;  // multiple input cells found
      }
    }
    return true;
  }

  bool update(const Operation &op, bool after) {
    if (op.type == Operation::Type::LPB) {
      open_loops++;
    } else if (op.type == Operation::Type::LPE) {
      open_loops--;
    }
    const auto meta = Operation::Metadata::get(op.type);
    // check the source cell
    if (meta.num_operands > 1 && op.source.type == Operand::Type::DIRECT) {
      if (!read(op.source.value.asInt(), after)) {
        return false;
      }
    }
    // check the target cell
    if (meta.num_operands > 0 && op.target.type == Operand::Type::DIRECT) {
      auto target = op.target.value.asInt();
      if (meta.is_reading_target && !read(op.target.value.asInt(), after)) {
        return false;
      }
      if (!after && meta.is_writing_target) {
        written_cells.insert(target);
      }
    }
    return true;
  }

  void reset(bool after) {
    if (!after) {
      input_cell = -1;
      written_cells.clear();
    }
    output_cell = -1;
  }
};

std::vector<EmbeddedSequenceProgram> Subprogram::findEmbeddedSequencePrograms(
    const Program &p, int64_t min_length) {
  std::vector<EmbeddedSequenceProgram> result;
  const int64_t num_ops = p.ops.size();
  if (num_ops == 0 || ProgramUtil::hasIndirectOperand(p)) {
    return result;
  }
  CellTracker tracker;
  for (int64_t start = 0; start + 1 < num_ops; start++) {
    tracker.reset(false);
    int64_t end = start - 1;
    for (int64_t i = start; i < num_ops; i++) {
      bool ok = tracker.update(p.ops[i], false) && tracker.open_loops == 0;
      if (ok) {
        // check rest of program
        tracker.reset(true);
        for (int64_t j = i + 1; j < num_ops; j++) {
          if (!tracker.update(p.ops[i], true)) {
            ok = false;
            break;
          }
        }
      }
      if (ok) {
        end++;
      } else {
        break;
      }
    }
    if (start + min_length <= end) {
      EmbeddedSequenceProgram esp;
      esp.start_pos = start;
      esp.end_pos = end;
      esp.input_cell = tracker.input_cell;
      esp.output_cell = tracker.output_cell;
      result.emplace_back(esp);
    }
  }
  return result;
}
