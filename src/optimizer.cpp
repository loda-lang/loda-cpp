#include "optimizer.hpp"

#include <map>
#include <set>
#include <stack>

#include "interpreter.hpp"
#include "number.hpp"
#include "program_util.hpp"
#include "semantics.hpp"
#include "util.hpp"

bool Optimizer::optimize(Program &p) const {
  if (Log::get().level == Log::Level::DEBUG) {
    Log::get().debug("Starting optimization of program with " +
                     std::to_string(p.ops.size()) + " operations");
  }
  bool changed = true;
  bool result = false;
  while (changed) {
    changed = false;
    if (simplifyOperations(p)) {
      changed = true;
    }
    if (mergeOps(p)) {
      changed = true;
    }
    if (removeNops(p)) {
      changed = true;
    }
    if (removeEmptyLoops(p)) {
      changed = true;
    }
    if (reduceMemoryCells(p)) {
      changed = true;
    }
    if (partialEval(p)) {
      changed = true;
    }
    if (sortOperations(p)) {
      changed = true;
    }
    if (mergeLoops(p)) {
      changed = true;
    }
    if (pullUpMov(p)) {
      changed = true;
    }
    result = result || changed;
  }
  if (Log::get().level == Log::Level::DEBUG) {
    Log::get().debug("Finished optimization; program now has " +
                     std::to_string(p.ops.size()) + " operations");
  }
  return result;
}

bool Optimizer::removeNops(Program &p) const {
  bool removed = false;
  auto it = p.ops.begin();
  while (it != p.ops.end()) {
    if (ProgramUtil::isNop(*it)) {
      if (Log::get().level == Log::Level::DEBUG) {
        Log::get().debug("Removing nop operation");
      }
      it = p.ops.erase(it);
      removed = true;
    } else {
      ++it;
    }
  }
  return removed;
}

bool Optimizer::removeEmptyLoops(Program &p) const {
  bool removed = false;
  for (int64_t i = 0; i < static_cast<int64_t>(p.ops.size());
       i++)  // need to use signed integers here
  {
    if (i + 1 < static_cast<int64_t>(p.ops.size()) &&
        p.ops[i].type == Operation::Type::LPB &&
        p.ops[i + 1].type == Operation::Type::LPE) {
      if (Log::get().level == Log::Level::DEBUG) {
        Log::get().debug("Removing empty loop");
      }
      p.ops.erase(p.ops.begin() + i, p.ops.begin() + i + 2);
      i = i - 2;
      removed = true;
    }
  }
  return removed;
}

bool Optimizer::mergeOps(Program &p) const {
  bool merged = false;
  for (size_t i = 0; i + 1 < p.ops.size(); i++) {
    bool do_merge = false;

    auto &o1 = p.ops[i];
    auto &o2 = p.ops[i + 1];

    // operation targets the direct same?
    if (o1.target == o2.target && o1.target.type == Operand::Type::DIRECT) {
      // both sources constants?
      if (o1.source.type == Operand::Type::CONSTANT &&
          o2.source.type == Operand::Type::CONSTANT) {
        // both add or sub operation?
        if (o1.type == o2.type && (o1.type == Operation::Type::ADD ||
                                   o1.type == Operation::Type::SUB)) {
          o1.source.value = Semantics::add(o1.source.value, o2.source.value);
          do_merge = true;
        }

        // both mul, div or pow operations?
        else if (o1.type == o2.type && (o1.type == Operation::Type::MUL ||
                                        o1.type == Operation::Type::DIV ||
                                        o1.type == Operation::Type::POW)) {
          o1.source.value = Semantics::mul(o1.source.value, o2.source.value);
          do_merge = true;
        }

        // one add, the other sub?
        else if ((o1.type == Operation::Type::ADD &&
                  o2.type == Operation::Type::SUB) ||
                 (o1.type == Operation::Type::SUB &&
                  o2.type == Operation::Type::ADD)) {
          o1.source.value = Semantics::sub(o1.source.value, o2.source.value);
          if (o1.source.value < Number::ZERO) {
            o1.source.value = Semantics::sub(Number::ZERO, o1.source.value);
            o1.type = (o1.type == Operation::Type::ADD) ? Operation::Type::SUB
                                                        : Operation::Type::ADD;
          }
          do_merge = true;
        }

        // first sub, the other max?
        else if (o1.type == Operation::Type::SUB &&
                 o2.type == Operation::Type::MAX &&
                 o2.source.type == Operand::Type::CONSTANT &&
                 o2.source.value == 0) {
          o1.type = Operation::Type::TRN;
          do_merge = true;
        }

        // first mul, second div?
        else if (o1.type == Operation::Type::MUL &&
                 o2.type == Operation::Type::DIV &&
                 o1.source.value != Number::ZERO &&
                 o2.source.value != Number::ZERO &&
                 Semantics::mod(o1.source.value, o2.source.value) ==
                     Number::ZERO) {
          o1.source.value = Semantics::div(o1.source.value, o2.source.value);
          do_merge = true;
        }

        // both sources same memory cell?
        else if (o1.source.type == Operand::Type::DIRECT &&
                 o1.source == o2.source) {
          // add with inverse sub?
          if (o1.type == Operation::Type::ADD &&
              o2.type == Operation::Type::SUB) {
            o1.source = Operand(Operand::Type::CONSTANT, 0);
            do_merge = true;
          }
        }

      }

      // sources the same direct access?
      else if (o1.source.type == Operand::Type::DIRECT &&
               o2.source == o1.source) {
        // add / sub combination?
        if ((o1.type == Operation::Type::ADD &&
             o2.type == Operation::Type::SUB) ||
            (o1.type == Operation::Type::SUB &&
             o2.type == Operation::Type::ADD)) {
          o1.source = Operand(Operand::Type::CONSTANT, 0);
          do_merge = true;
        }
      }

      // first operation mov with constant?
      if (!do_merge && o1.type == Operation::Type::MOV &&
          o1.source.type == Operand::Type::CONSTANT &&
          o2.source.type != Operand::Type::INDIRECT) {
        // mov to 0, then add another cell => mov directly
        // mov to 1, then mul another cell => mov directly
        if ((o1.source.value == 0 && o2.type == Operation::Type::ADD) ||
            (o1.source.value == 1 && o2.type == Operation::Type::MUL)) {
          o1.source = o2.source;
          do_merge = true;
        }
      }

      // second operation mov with constant?
      if (!do_merge && o2.type == Operation::Type::MOV &&
          o2.source.type == Operand::Type::CONSTANT) {
        // first operation writing target?
        if (Operation::Metadata::get(o1.type).is_writing_target &&
            o1.type != Operation::Type::CLR) {
          // second mov overwrites first operation
          o1 = o2;
          do_merge = true;
        }
      }
    }

    // merge (erase second operation)
    if (do_merge) {
      if (Log::get().level == Log::Level::DEBUG) {
        Log::get().debug("Merging similar consecutive operation");
      }
      p.ops.erase(p.ops.begin() + i + 1, p.ops.begin() + i + 2);
      --i;
      merged = true;
    }
  }

  return merged;
}

inline bool simplifyOperand(Operand &op,
                            std::unordered_set<int64_t> &initialized_cells,
                            bool is_source) {
  switch (op.type) {
    case Operand::Type::CONSTANT:
      break;
    case Operand::Type::DIRECT:
      if (initialized_cells.find(op.value.asInt()) == initialized_cells.end() &&
          is_source) {
        op.type = Operand::Type::CONSTANT;
        op.value = 0;
        return true;
      }
      break;
    case Operand::Type::INDIRECT:
      if (initialized_cells.find(op.value.asInt()) == initialized_cells.end()) {
        op.type = Operand::Type::DIRECT;
        op.value = 0;
        return true;
      }
      break;
  }
  return false;
}

bool Optimizer::simplifyOperations(Program &p) const {
  std::unordered_set<int64_t> initialized_cells;
  for (size_t i = 0; i < NUM_INITIALIZED_CELLS; ++i) {
    initialized_cells.insert(i);
  }
  bool simplified = false;
  bool can_simplify = true;
  for (auto &op : p.ops) {
    switch (op.type) {
      case Operation::Type::NOP:
      case Operation::Type::DBG:
        break;  // can be safely ignored

      case Operation::Type::LPB:
      case Operation::Type::LPE:
      case Operation::Type::CLR:
      case Operation::Type::SEQ:
        can_simplify = false;
        break;

      default: {
        if (can_simplify) {
          // simplify operands
          bool has_source = Operation::Metadata::get(op.type).num_operands == 2;
          if (has_source &&
              simplifyOperand(op.source, initialized_cells, true)) {
            simplified = true;
          }
          if (simplifyOperand(op.target, initialized_cells, false)) {
            simplified = true;
          }

          // simplify operation: target uninitialized (cell content matters!)
          if (op.target.type == Operand::Type::DIRECT &&
              initialized_cells.find(op.target.value.asInt()) ==
                  initialized_cells.end()) {
            // add $n,X => mov $n,X (where $n is uninitialized)
            if (op.type == Operation::Type::ADD) {
              op.type = Operation::Type::MOV;
              simplified = true;
            }
          }
        }

        // simplify operation: source is zero (cell content doesn't matter)
        if (op.source.type == Operand::Type::CONSTANT && op.source.value == 0) {
          // trn $n,0 => max $n,0
          if (op.type == Operation::Type::TRN) {
            op.type = Operation::Type::MAX;
            simplified = true;
          }
        }

        // simplify operation: source is negative constant (cell content doesn't
        // matter)
        if (op.source.type == Operand::Type::CONSTANT && op.source.value < 0) {
          // add $n,-k => sub $n,k
          if (op.type == Operation::Type::ADD) {
            op.type = Operation::Type::SUB;
            op.source.value = Semantics::sub(Number::ZERO, op.source.value);
            simplified = true;
          }
          // sub $n,-k => add $n,k
          else if (op.type == Operation::Type::SUB) {
            op.type = Operation::Type::ADD;
            op.source.value = Semantics::sub(Number::ZERO, op.source.value);
            simplified = true;
          }
        }

        // simplify operation: target equals source (cell content doesn't
        // matter)
        if (op.target.type == Operand::Type::DIRECT && op.target == op.source) {
          // add $n,$n => mul $n,2
          if (op.type == Operation::Type::ADD) {
            op.type = Operation::Type::MUL;
            op.source = Operand(Operand::Type::CONSTANT, 2);
            simplified = true;
          }
          // sub $n,$n => mov $n,0
          else if (op.type == Operation::Type::SUB) {
            op.type = Operation::Type::MOV;
            op.source = Operand(Operand::Type::CONSTANT, 0);
            simplified = true;
          }
          // mul $n,$n => pow $n,2
          else if (op.type == Operation::Type::MUL) {
            op.type = Operation::Type::POW;
            op.source = Operand(Operand::Type::CONSTANT, 2);
            simplified = true;
          }
          // cmp $n,$n / bin $n,$n => mov $n,1
          else if (op.type == Operation::Type::CMP ||
                   op.type == Operation::Type::BIN) {
            op.type = Operation::Type::MOV;
            op.source = Operand(Operand::Type::CONSTANT, 1);
            simplified = true;
          }
        }

        // update initialized cells
        switch (op.target.type) {
          case Operand::Type::DIRECT:
            initialized_cells.insert(op.target.value.asInt());
            break;
          case Operand::Type::INDIRECT:
            can_simplify =
                false;  // don't know at this point which cell is written to
            break;
          case Operand::Type::CONSTANT:
            Log::get().error("invalid program");
            break;
        }
        break;
      }
    }
  }
  if (simplified && Log::get().level == Log::Level::DEBUG) {
    Log::get().debug("Simplifying operations");
  }
  return simplified;
}

bool Optimizer::canChangeVariableOrder(const Program &p) const {
  for (auto &op : p.ops) {
    if (op.source.type == Operand::Type::INDIRECT ||
        op.target.type == Operand::Type::INDIRECT) {
      return false;
    }
    if (op.type == Operation::Type::LPB || op.type == Operation::Type::CLR) {
      if (op.source.type != Operand::Type::CONSTANT || op.source.value != 1) {
        return false;
      }
    }
  }
  return true;
}

bool Optimizer::reduceMemoryCells(Program &p) const {
  std::unordered_set<int64_t> used_cells;
  int64_t largest_used = 0;
  if (!canChangeVariableOrder(p)) {
    return false;
  }
  if (!ProgramUtil::getUsedMemoryCells(p, used_cells, largest_used,
                                       settings.max_memory)) {
    return false;
  }
  for (int64_t candidate = 0; candidate < largest_used; ++candidate) {
    bool free = true;
    if (candidate < static_cast<int64_t>(NUM_RESERVED_CELLS)) {
      free = false;
    }
    for (int64_t used : used_cells) {
      if (used == candidate) {
        free = false;
        break;
      }
    }
    if (free) {
      bool replaced = false;
      for (auto &op : p.ops) {
        if (op.source.type == Operand::Type::DIRECT &&
            op.source.value == largest_used) {
          op.source.value = candidate;
          replaced = true;
        }
        if (op.target.type == Operand::Type::DIRECT &&
            op.target.value == largest_used) {
          op.target.value = candidate;
          replaced = true;
        }
      }
      if (replaced && Log::get().level == Log::Level::DEBUG) {
        Log::get().debug("Reducing memory cell");
      }
      return replaced;
    }
  }
  return false;
}

void removeReferences(const Operand &op, std::map<int64_t, Operand> &values) {
  auto it = values.begin();
  // do we need to do this transitively?
  while (it != values.end()) {
    if (it->second == op) {
      it = values.erase(it);
    } else {
      it++;
    }
  }
}

bool Optimizer::doPartialEval(Program &p, size_t op_index,
                              std::map<int64_t, Operand> &values) const {
  // make sure there is not indirect memory access
  Operation &op = p.ops[op_index];
  if (ProgramUtil::hasIndirectOperand(op)) {
    values.clear();
    return false;
  }

  // deduce source value
  auto source = op.source;
  if (op.source.type == Operand::Type::DIRECT &&
      values.find(op.source.value.asInt()) != values.end()) {
    source = values[op.source.value.asInt()];
  }

  // deduce target value
  auto target = op.target;
  if (op.target.type == Operand::Type::DIRECT &&
      values.find(op.target.value.asInt()) != values.end()) {
    target = values[op.target.value.asInt()];
  }

  // calculate new value
  bool has_result = false;
  auto num_ops = Operation::Metadata::get(op.type).num_operands;
  switch (op.type) {
    case Operation::Type::NOP:
    case Operation::Type::DBG:
    case Operation::Type::SEQ: {
      break;
    }

    case Operation::Type::LPB:
    case Operation::Type::LPE: {
      // remove values from cells that are modified in the loop
      auto loop = ProgramUtil::getEnclosingLoop(p, op_index);
      for (int64_t i = loop.first + 1; i < loop.second; i++) {
        if (p.ops[i].type == Operation::Type::CLR ||
            ProgramUtil::hasIndirectOperand(p.ops[i])) {
          values.clear();
          break;
        }
        if (Operation::Metadata::get(p.ops[i].type).is_writing_target) {
          values.erase(p.ops[i].target.value.asInt());
          removeReferences(p.ops[i].target, values);
        }
      }
      return false;
    }
    case Operation::Type::CLR: {
      values.clear();
      return false;
    }

    case Operation::Type::MOV: {
      target = source;
      has_result = true;
      break;
    }

    default: {
      if (target.type == Operand::Type::CONSTANT &&
          (num_ops == 1 || source.type == Operand::Type::CONSTANT)) {
        target.value = Interpreter::calc(op.type, target.value, source.value);
        has_result = true;
      }
      break;
    }
  }

  bool changed = false;

  // update source value
  if (num_ops == 2 && op.source != source) {
    op.source = source;
    changed = true;
  }

  if (num_ops > 0) {
    // update target value
    if (has_result) {
      values[op.target.value.asInt()] = target;
      if (op.type != Operation::Type::MOV) {
        op.type = Operation::Type::MOV;
        op.source = target;
        changed = true;
      }
    } else {
      values.erase(op.target.value.asInt());
    }

    // remove references to target because they are out-dated now
    removeReferences(op.target, values);
  }

  return changed;
}

bool Optimizer::partialEval(Program &p) const {
  std::unordered_set<int64_t> used_cells;
  int64_t largest_used = 0;
  if (!ProgramUtil::getUsedMemoryCells(p, used_cells, largest_used,
                                       settings.max_memory)) {
    return false;
  }
  std::map<int64_t, Operand> values;
  for (int64_t i = NUM_INITIALIZED_CELLS; i <= largest_used; i++) {
    values[i] = Operand(Operand::Type::CONSTANT, 0);
  }
  bool changed = false;
  for (size_t i = 0; i < p.ops.size(); i++) {
    if (doPartialEval(p, i, values)) {
      changed = true;
    }
  }
  return changed;
}

bool Optimizer::shouldSwapOperations(const Operation &first,
                                     const Operation &second) const {
  // check if we can swap
  if (!ProgramUtil::areIndependent(first, second)) {
    return false;
  }
  // check if we should swap
  if (first.target.value < second.target.value) {
    return false;
  }
  if (second.target.value < first.target.value) {
    return true;
  }
  return false;
}

bool Optimizer::sortOperations(Program &p) const {
  bool changed = false;
  for (int64_t i = 1; i < static_cast<int64_t>(p.ops.size()); i++) {
    for (int64_t j = i - 1; j >= 0; j--) {
      if (shouldSwapOperations(p.ops[j], p.ops[j + 1])) {
        std::swap(p.ops[j], p.ops[j + 1]);
        changed = true;
      } else {
        break;
      }
    }
  }
  return changed;
}

void throwInvalidLoop() {
  throw std::runtime_error("invalid loop detected during optimization");
}

bool Optimizer::mergeLoops(Program &p) const {
  std::stack<size_t> loop_begins;
  for (size_t i = 0; i + 1 < p.ops.size(); i++) {
    if (p.ops[i].type == Operation::Type::LPB) {
      loop_begins.push(i);
    } else if (p.ops[i].type == Operation::Type::LPE) {
      if (loop_begins.empty()) {
        throwInvalidLoop();
      }
      auto lpb2 = loop_begins.top();
      loop_begins.pop();
      if (p.ops[i + 1].type == Operation::Type::LPE) {
        if (loop_begins.empty()) {
          throwInvalidLoop();
        }
        auto lpb1 = loop_begins.top();
        if (lpb1 + 1 == lpb2 && p.ops[lpb1] == p.ops[lpb2]) {
          p.ops.erase(p.ops.begin() + i, p.ops.begin() + i + 1);
          p.ops.erase(p.ops.begin() + lpb1, p.ops.begin() + lpb1 + 1);
          return true;
        }
      }
    }
  }
  return false;
}

/*
 * Returns true if mergeOps() can merge these two operation types.
 */
bool canMerge(Operation::Type a, Operation::Type b) {
  if ((a == Operation::Type::ADD || a == Operation::Type::SUB) &&
      (b == Operation::Type::ADD || b == Operation::Type::SUB)) {
    return true;
  }
  if (a == b && (a == Operation::Type::MUL || a == Operation::Type::DIV)) {
    return true;
  }
  if (a == Operation::Type::MUL && b == Operation::Type::DIV) {
    return true;
  }
  return false;
}

bool Optimizer::pullUpMov(Program &p) const {
  // see tests E014 and E015
  bool changed = false;
  for (size_t i = 0; i + 2 < p.ops.size(); i++) {
    auto &a = p.ops[i];
    auto &b = p.ops[i + 1];
    auto &c = p.ops[i + 2];
    // check operation types
    if (!canMerge(a.type, c.type)) {
      continue;
    }
    if (b.type != Operation::Type::MOV) {
      continue;
    }
    // check operand types
    if (a.target.type != Operand::Type::DIRECT ||
        a.source.type != Operand::Type::CONSTANT ||
        b.target.type != Operand::Type::DIRECT ||
        b.source.type != Operand::Type::DIRECT ||
        c.target.type != Operand::Type::DIRECT ||
        c.source.type != Operand::Type::CONSTANT) {
      continue;
    }
    // check operand values
    if (a.target.value != b.source.value || b.target.value != c.target.value) {
      continue;
    }
    // okay, we are ready to optimize!
    Operation d = a;
    d.target.value = b.target.value;
    std::swap(a, b);
    p.ops.insert(p.ops.begin() + i + 1, d);
    changed = true;
  }
  return changed;
}

// === OperationMover ====================================

void Optimizer::OperationMover::init(Program *prog) {
  this->prog = prog;
  opScores.resize(prog->ops.size());
  std::fill(opScores.begin(), opScores.end(), 0);
  totalScore = 0;
  for (size_t i = 0; i + 1 < prog->ops.size(); i++) {
    updateScore(i);
  }
}

void Optimizer::OperationMover::updateScore(size_t i) {
  const auto score = scoreNeighbors(i);
  totalScore += score - opScores[i];
  opScores[i] = score;
}

void Optimizer::OperationMover::updateNeighborhood(size_t i) {
  if (i > 0) {
    updateScore(i - 1);
  }
  updateScore(i);
  if (i + 1 < prog->ops.size()) {
    updateScore(i + 1);
  }
}

bool Optimizer::OperationMover::up(size_t i) {
  if (i == 0) {
    return false;
  }
  std::swap(prog->ops[i - 1], prog->ops[i]);
  updateNeighborhood(i - 1);
  return true;
}

bool Optimizer::OperationMover::down(size_t i) {
  if (i + 1 == prog->ops.size()) {
    return false;
  }
  std::swap(prog->ops[i], prog->ops[i + 1]);
  updateNeighborhood(i);
  return true;
}

int64_t Optimizer::OperationMover::scoreNeighbors(size_t i) const {
  const Operation &op1 = prog->ops[i];
  const Operation &op2 = prog->ops[i + 1];
  int64_t score = 0;
  if (op1.target == op2.target) {
    score += 4;
    if (op1.source.type == op2.source.type) {
      score += 3;
      if (canMerge(op1.type, op2.type)) {
        score += 2;
      }
    }
  } else if (op1.target.value < op2.target.value) {
    score += 1;
  }
  return score;
}
