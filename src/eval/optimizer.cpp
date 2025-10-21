#include "eval/optimizer.hpp"

#include <map>
#include <set>
#include <stack>

#include "eval/evaluator_par.hpp"
#include "eval/interpreter.hpp"
#include "eval/semantics.hpp"
#include "lang/program_util.hpp"
#include "sys/log.hpp"
#include "sys/util.hpp"

bool Optimizer::optimize(Program &p) const {
  if (Log::get().level == Log::Level::DEBUG) {
    Log::get().debug("Starting optimization of program with " +
                     std::to_string(p.ops.size()) + " operations");
  }
  bool changed = true;
  bool result = false;
  while (changed) {
    changed = false;
    if (collapseMovChains(p)) {
      changed = true;
    }
    if (simplifyOperations(p)) {
      changed = true;
    }
    {
      // attention: fixSandwich() should be executed directly before mergeOps()
      if (fixSandwich(p)) {
        changed = true;
      }
      if (mergeOps(p)) {
        changed = true;
      }
      if (mergeRepeated(p)) {
        changed = true;
      }
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
    if (collapseMovLoops(p)) {
      changed = true;
    }
    if (collapseDifLoops(p)) {
      changed = true;
    }
    if (collapseArithmeticLoops(p)) {
      changed = true;
    }
    if (pullUpMov(p)) {
      changed = true;
    }
    if (removeCommutativeDetour(p)) {
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
      i = std::max<int64_t>(i - 2, 0);
      removed = true;
    }
  }
  return removed;
}

bool Optimizer::mergeOps(Program &p) const {
  bool updated = false;
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
                 o2.source.value != Number::ZERO) {
          auto gcd = Semantics::gcd(o1.source.value, o2.source.value);
          o1.source.value = Semantics::div(o1.source.value, gcd);
          if (gcd == o2.source.value) {
            do_merge = true;
          } else if (gcd != Number::ONE) {
            o2.source.value = Semantics::div(o2.source.value, gcd);
            updated = true;
          }
        }

        // first pow, second nrt?
        else if (o1.type == Operation::Type::POW &&
                 o2.type == Operation::Type::NRT &&
                 o1.source.value > Number::ONE &&
                 o2.source.value > Number::ONE) {
          auto gcd = Semantics::gcd(o1.source.value, o2.source.value);
          auto new_o1_value = Semantics::div(o1.source.value, gcd);
          // Special case: if exponents are equal and result would be a no-op,
          // handle differently based on whether exponent is even or odd
          if (new_o1_value == Number::ONE) {
            // If exponent is even: pow $0,k; nrt $0,k computes abs($0)
            // Replace with gcd $0,0 which is more efficient
            if (!o1.source.value.odd()) {
              o1.type = Operation::Type::GCD;
              o1.source = Operand(Operand::Type::CONSTANT, 0);
              do_merge = true;
            }
            // If exponent is odd: pow $0,k; nrt $0,k is a no-op for non-negative
            // values but causes overflow for negative values. Keep the operations
            // to preserve the original behavior.
          } else {
            o1.source.value = new_o1_value;
            if (gcd == o2.source.value) {
              do_merge = true;
            } else if (gcd != Number::ONE) {
              o2.source.value = Semantics::div(o2.source.value, gcd);
              updated = true;
            }
          }
        }

        // first mul, second dgs?
        else if (o1.type == Operation::Type::MUL &&
                 o2.type == Operation::Type::DGS &&
                 o1.source.value > Number::ZERO &&
                 o2.source.value > Number::ZERO &&
                 Semantics::getPowerOf(o1.source.value, o2.source.value) !=
                     Number::ZERO) {
          o1 = o2;
          do_merge = true;
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
            !ProgramUtil::isWritingRegion(o1.type)) {
          // second mov overwrites first operation
          o1 = o2;
          do_merge = true;
        }
      }

      // equ X,Y ; equ X,0 => neq X,Y
      if (!do_merge && o1.type == Operation::Type::EQU &&
          o2.type == Operation::Type::EQU &&
          o2.source.type == Operand::Type::CONSTANT &&
          o2.source.value == Number::ZERO) {
        o1.type = Operation::Type::NEQ;
        do_merge = true;
      }
    }

    // merge (erase second operation)
    if (do_merge) {
      if (Log::get().level == Log::Level::DEBUG) {
        Log::get().debug("Merging similar consecutive operations");
      }
      p.ops.erase(p.ops.begin() + i + 1, p.ops.begin() + i + 2);
      --i;
      updated = true;
    }
  }

  return updated;
}

std::pair<int64_t, int64_t> findRepeatedOps(const Program &p,
                                            int64_t min_repetitions) {
  std::pair<int64_t, int64_t> pos(-1, 0);  // start, length
  for (size_t i = 0; i < p.ops.size(); i++) {
    if (pos.first != -1) {  // start found already
      if (p.ops[i] == p.ops[pos.first]) {
        // another repetition
        pos.second++;
      } else {
        // reached end
        if (pos.second >= min_repetitions) {
          return pos;
        }
        pos.first = -1;
        pos.second = 0;
      }
    }
    if (pos.first == -1 && (p.ops[i].type == Operation::Type::ADD ||
                            p.ops[i].type == Operation::Type::MUL)) {
      pos.first = i;   // new start found
      pos.second = 1;  // one operation so far
    }
  }
  // final check
  if (pos.second < min_repetitions) {
    pos.first = -1;
  }
  return pos;
}

std::pair<int64_t, int64_t> findConsecutiveMovOps(const Program &p,
                                                  int64_t min_repetitions) {
  std::pair<int64_t, int64_t> pos(-1, 0);  // start, length
  for (size_t i = 0; i < p.ops.size(); i++) {
    const auto &op = p.ops[i];
    if (op.type == Operation::Type::MOV &&
        op.target.type == Operand::Type::DIRECT &&
        op.source.type == Operand::Type::CONSTANT) {
      if (pos.first == -1) {
        // start a new sequence
        pos.first = i;
        pos.second = 1;
      } else {
        // check if this continues the sequence
        const auto &first_op = p.ops[pos.first];
        const auto &prev_op = p.ops[i - 1];
        // must have same source value and consecutive target cells
        if (op.source == first_op.source &&
            op.target.value.asInt() == prev_op.target.value.asInt() + 1) {
          pos.second++;
        } else {
          // sequence ended
          if (pos.second >= min_repetitions) {
            return pos;
          }
          // try starting a new sequence from this operation
          pos.first = i;
          pos.second = 1;
        }
      }
    } else {
      // not a mov operation, check if we have a valid sequence
      if (pos.first != -1 && pos.second >= min_repetitions) {
        return pos;
      }
      pos.first = -1;
      pos.second = 0;
    }
  }
  // final check
  if (pos.second < min_repetitions) {
    pos.first = -1;
  }
  return pos;
}

bool Optimizer::mergeRepeated(Program &p) const {
  // merge consecutive mov operations into fil/clr
  auto mov_pos = findConsecutiveMovOps(p, 3);
  if (mov_pos.first != -1) {
    const auto &first_mov = p.ops[mov_pos.first];
    Operand count(Operand::Type::CONSTANT, mov_pos.second);
    bool use_clr = (first_mov.source.type == Operand::Type::CONSTANT &&
                    first_mov.source.value == Number::ZERO);
    if (Log::get().level == Log::Level::DEBUG) {
      Log::get().debug("Merging " + std::to_string(mov_pos.second) +
                       " consecutive mov operations");
    }
    if (use_clr) {
      p.ops[mov_pos.first] =
          Operation(Operation::Type::CLR, first_mov.target, count);
    } else {
      p.ops[mov_pos.first + 1] =
          Operation(Operation::Type::FIL, first_mov.target, count);
    }
    // erase remaining operations
    int64_t erase_offset = use_clr ? 1 : 2;
    int64_t erase_start = mov_pos.first + erase_offset;
    if (mov_pos.second > erase_offset) {
      p.ops.erase(p.ops.begin() + erase_start,
                  p.ops.begin() + mov_pos.first + mov_pos.second);
    }
    return true;
  }

  // check for repeated add/mul operations
  auto pos = findRepeatedOps(p, 3);
  if (pos.first == -1) {
    return false;
  }
  if (ProgramUtil::hasIndirectOperand(p)) {
    return false;
  }
  Operation::Type merge_type = (p.ops[pos.first].type == Operation::Type::ADD)
                                   ? Operation::Type::MUL
                                   : Operation::Type::POW;
  int64_t largest = 0;
  if (!ProgramUtil::getUsedMemoryCells(p, nullptr, nullptr, largest,
                                       settings.max_memory)) {
    return false;
  }
  Operand tmp_cell(Operand::Type::DIRECT, largest + 1);
  Operand count(Operand::Type::CONSTANT, pos.second);
  p.ops[pos.first].type = Operation::Type::MOV;
  p.ops[pos.first].target = tmp_cell;
  p.ops[pos.first + 1] = Operation(merge_type, tmp_cell, count);
  p.ops[pos.first + 2].source = tmp_cell;
  if (pos.second > 3) {
    p.ops.erase(p.ops.begin() + pos.first + 3,
                p.ops.begin() + pos.first + pos.second);
  }
  return true;
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
      case Operation::Type::FIL:
      case Operation::Type::ROL:
      case Operation::Type::ROR:
      case Operation::Type::PRG:
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
          // mul $n,0 => mov $n,0
          if (op.type == Operation::Type::MUL) {
            op.type = Operation::Type::MOV;
            simplified = true;
          }
          // pow $n,0 => mov $n,1
          if (op.type == Operation::Type::POW) {
            op.type = Operation::Type::MOV;
            op.source = Operand(Operand::Type::CONSTANT, 1);
            simplified = true;
          }
          // fac $n,0 => mov $n,1
          if (op.type == Operation::Type::FAC) {
            op.type = Operation::Type::MOV;
            op.source = Operand(Operand::Type::CONSTANT, 1);
            simplified = true;
          }
          // ban $n,0 => mov $n,0
          if (op.type == Operation::Type::BAN) {
            op.type = Operation::Type::MOV;
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
          // equ $n,$n / leq $n,$n / geq $n,$n / bin $n,$n => mov $n,1
          else if (op.type == Operation::Type::EQU ||
                   op.type == Operation::Type::LEQ ||
                   op.type == Operation::Type::GEQ ||
                   op.type == Operation::Type::BIN) {
            op.type = Operation::Type::MOV;
            op.source = Operand(Operand::Type::CONSTANT, 1);
            simplified = true;
          }
          // neq $n,$n => mov $n,0
          else if (op.type == Operation::Type::NEQ) {
            op.type = Operation::Type::MOV;
            op.source = Operand(Operand::Type::CONSTANT, 0);
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

bool Optimizer::fixSandwich(Program &p) const {
  bool changed = false;
  for (size_t i = 0; i + 2 < p.ops.size(); i++) {
    auto &op1 = p.ops[i];
    auto &op2 = p.ops[i + 1];
    auto &op3 = p.ops[i + 2];
    if (op1.target != op2.target || op2.target != op3.target ||
        op1.target.type != Operand::Type::DIRECT ||
        op1.source.type != Operand::Type::CONSTANT ||
        op2.source.type != Operand::Type::CONSTANT ||
        op3.source.type != Operand::Type::CONSTANT) {
      continue;
    }
    if (ProgramUtil::isAdditive(op1.type) && op2.type == Operation::Type::MUL &&
        ProgramUtil::isAdditive(op3.type)) {
      std::swap(op1, op2);
      op2.source.value *= op1.source.value;
      changed = true;
    } else if (ProgramUtil::isAdditive(op2.type) && op1.type == op3.type) {
      if (op1.type == Operation::Type::DIV) {
        std::swap(op1, op2);
        op1.source.value *= op2.source.value;
        changed = true;
      } else if (op1.type == Operation::Type::MUL &&
                 Semantics::mod(op2.source.value, op1.source.value) ==
                     Number::ZERO) {
        std::swap(op1, op2);
        op1.source.value /= op2.source.value;
        changed = true;
      }
    } else if (ProgramUtil::isAdditive(op2.type) &&
               op1.type == Operation::Type::MUL &&
               op3.type == Operation::Type::DIV && op1.source == op3.source &&
               Number::ONE < op1.source.value &&
               Number::ONE < op2.source.value &&
               Semantics::mod(op2.source.value, op1.source.value) ==
                   Number::ZERO) {
      std::swap(op1, op2);
      op1.source.value /= op2.source.value;
      changed = true;
    }
  }
  return changed;
}

bool Optimizer::canChangeVariableOrder(const Program &p) const {
  return (std::none_of(p.ops.begin(), p.ops.end(), [&](const Operation &op) {
    return ProgramUtil::hasIndirectOperand(op) ||
           ProgramUtil::isNonTrivialLoopBegin(op) ||
           ProgramUtil::isNonTrivialClear(op) ||
           ProgramUtil::isWritingRegion(op.type);
  }));
}

bool Optimizer::reduceMemoryCells(Program &p) const {
  std::unordered_set<int64_t> used_cells;
  int64_t largest_used = 0;
  if (!canChangeVariableOrder(p)) {
    return false;
  }
  if (!ProgramUtil::getUsedMemoryCells(p, nullptr, &used_cells, largest_used,
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

bool Optimizer::partialEval(Program &p) const {
  int64_t largest_used = 0;
  if (!ProgramUtil::getUsedMemoryCells(p, nullptr, nullptr, largest_used,
                                       settings.max_memory)) {
    return false;
  }
  PartialEvaluator eval(settings);
  eval.initZeros(NUM_INITIALIZED_CELLS, largest_used);
  bool changed = false;
  for (size_t i = 0; i < p.ops.size(); i++) {
    bool has_result = eval.doPartialEval(p, i);
    auto &op = p.ops[i];
    auto source = eval.resolveOperand(op.source);
    auto target = eval.resolveOperand(op.target);
    auto num_ops = Operation::Metadata::get(op.type).num_operands;
    // update source operand
    if (num_ops > 1 && op.source != source) {
      op.source = source;
      changed = true;
    }
    // update target operand
    if (num_ops > 0 && has_result && op.type != Operation::Type::MOV) {
      op.type = Operation::Type::MOV;
      op.source = target;
      changed = true;
    }
  }
  return changed;
}

bool Optimizer::sortOperations(Program &p) const {
  opMover.init(p);
  for (size_t i = 0; i < p.ops.size(); i++) {
    int64_t oldScore = 0, maxScore = 0;
    size_t j = 0, maxIndex = 0;
    j = i;
    oldScore = opMover.getTotalScore();
    while (opMover.up(j)) {
      j--;
    }
    maxIndex = j;
    maxScore = opMover.getTotalScore();
    while (opMover.down(j)) {
      j++;
      if (opMover.getTotalScore() > maxScore) {
        maxScore = opMover.getTotalScore();
        maxIndex = j;
      }
    }
    if (maxScore <= oldScore) {
      maxIndex = i;  // revert to old position
      maxScore = oldScore;
    }
    while (j != maxIndex) {
      opMover.up(j);
      j--;
    }
    if (maxScore != opMover.getTotalScore()) {
      Log::get().error("internal error sorting operations");
    }
    if (maxIndex != i) {
      return true;
    }
  }
  return false;
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

bool Optimizer::collapseMovLoops(Program &p) const {
  bool changed = false;
  for (size_t i = 0; i + 2 < p.ops.size(); i++) {
    if (p.ops[i].type != Operation::Type::LPB ||
        p.ops[i + 1].type != Operation::Type::MOV ||
        p.ops[i + 2].type != Operation::Type::LPE) {
      continue;
    }
    const auto &lpb = p.ops[i];
    const auto &mov = p.ops[i + 1];
    if (lpb.source != Operand(Operand::Type::CONSTANT, 1) ||
        lpb.target.type != Operand::Type::DIRECT ||
        mov.source.type != Operand::Type::CONSTANT ||
        mov.target != lpb.target) {
      continue;
    }
    auto val = mov.source.value;
    if (val < Number::ZERO) {
      p.ops.erase(p.ops.begin() + i, p.ops.begin() + i + 3);
      changed = true;
    } else {
      p.ops.erase(p.ops.begin() + i + 1, p.ops.begin() + i + 3);
      p.ops[i] = Operation(Operation::Type::MIN, lpb.target,
                           Operand(Operand::Type::CONSTANT, val));
      changed = true;
    }
  }
  return changed;
}

bool Optimizer::collapseDifLoops(Program &p) const {
  bool changed = false;
  for (size_t i = 0; i + 2 < p.ops.size(); i++) {
    if (p.ops[i].type != Operation::Type::LPB ||
        p.ops[i + 1].type != Operation::Type::DIF ||
        p.ops[i + 2].type != Operation::Type::LPE) {
      continue;
    }
    const auto &lpb = p.ops[i];
    const auto &dif = p.ops[i + 1];
    if (lpb.source != Operand(Operand::Type::CONSTANT, 1) ||
        lpb.target.type != Operand::Type::DIRECT ||
        dif.source.type != Operand::Type::CONSTANT ||
        dif.source.value < Number::ZERO || dif.target != lpb.target) {
      continue;
    }
    const auto val = dif.source.value;
    p.ops.erase(p.ops.begin() + i + 1, p.ops.begin() + i + 3);
    p.ops[i] = Operation(Operation::Type::DIR, lpb.target,
                         Operand(Operand::Type::CONSTANT, val));
    changed = true;
  }
  return changed;
}

bool Optimizer::collapseArithmeticLoops(Program &p) const {
  if (ProgramUtil::hasIndirectOperand(p)) {
    return false;
  }
  for (size_t i = 0; i + 3 < p.ops.size(); i++) {
    if (p.ops[i].type != Operation::Type::LPB) {  // must be loop start
      continue;
    }
    if (p.ops[i].source !=
        Operand(Operand::Type::CONSTANT, 1)) {  // must be simple loop
      continue;
    }
    const Operand loop_counter = p.ops[i].target;
    const Operation sub_test(Operation::Type::SUB, loop_counter,
                             Operand(Operand::Type::CONSTANT, 1));
    if (p.ops[i + 1] != sub_test) {  // must be "sub <loop_counter>,1"
      continue;
    }
    const Operation::Type basic_type = p.ops[i + 2].type;
    if (basic_type != Operation::Type::ADD &&
        basic_type != Operation::Type::MUL) {  // must be add or mul
      continue;
    }
    const Operand argument = p.ops[i + 2].source;
    const Operand target = p.ops[i + 2].target;
    if (argument == target || argument == loop_counter ||
        target == loop_counter) {  // argument, target, counter must be
                                   // different cells
      continue;
    }
    if (p.ops[i + 3].type != Operation::Type::LPE) {  // must be loop end
      continue;
    }
    // all checks passed, we can collapse the loop now
    const Operation::Type fold_type = (basic_type == Operation::Type::ADD)
                                          ? Operation::Type::MUL
                                          : Operation::Type::POW;
    int64_t largest = 0;
    if (!ProgramUtil::getUsedMemoryCells(p, nullptr, nullptr, largest,
                                         settings.max_memory)) {
      continue;
    }
    const Operand tmp_counter(Operand::Type::DIRECT, largest + 1);
    const Operand tmp_result(Operand::Type::DIRECT, largest + 2);
    p.ops[i] = Operation(Operation::Type::MOV, tmp_counter, loop_counter);
    p.ops[i + 1] = Operation(Operation::Type::MAX, tmp_counter,
                             Operand(Operand::Type::CONSTANT, 0));
    p.ops[i + 2] = Operation(Operation::Type::MOV, tmp_result, argument);
    p.ops[i + 3] = Operation(fold_type, tmp_result, tmp_counter);
    p.ops.insert(p.ops.begin() + i + 4,
                 Operation(basic_type, target, tmp_result));
    p.ops.insert(p.ops.begin() + i + 5,
                 Operation(Operation::Type::MIN, loop_counter,
                           Operand(Operand::Type::CONSTANT, 0)));
    return true;
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
    const auto &c = p.ops[i + 2];
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

bool isDirectMov(const Operation &op) {
  return op.type == Operation::Type::MOV &&
         op.target.type == Operand::Type::DIRECT &&
         op.source.type == Operand::Type::DIRECT;
}

Operation directMov(int64_t target, int64_t source) {
  return Operation(Operation::Type::MOV, Operand(Operand::Type::DIRECT, target),
                   Operand(Operand::Type::DIRECT, source));
}

bool Optimizer::collapseMovChains(Program &p) const {
  // Detect shift patterns in sequences of mov operations and replace with
  // rol/ror Left shift: mov $i,$i+1; mov $i+1,$i+2; ... => rol $i,length; mov
  // $end,$end+1 Right shift: mov $i,$i-1; mov $i-1,$i-2; ... => mov
  // $temp,$start; ror $start,length; mov $start,$temp
  bool changed = false;
  for (size_t i = 0; i + 1 < p.ops.size(); i++) {
    // Find first mov of chain
    if (!isDirectMov(p.ops[i])) {
      continue;
    }
    const auto &first_op = p.ops[i];
    int64_t first_target = first_op.target.value.asInt();
    int64_t first_source = first_op.source.value.asInt();
    int64_t direction =
        first_source - first_target;  // +1 for left, -1 for right
    if (direction != 1 && direction != -1) {
      continue;
    }

    // Count consecutive shift operations
    size_t shift_count = 1;
    int64_t last_target = first_target;
    for (size_t j = i + 1; j < p.ops.size(); j++) {
      if (!isDirectMov(p.ops[j])) {
        break;
      }
      int64_t curr_target = p.ops[j].target.value.asInt();
      int64_t curr_source = p.ops[j].source.value.asInt();
      if (curr_target == last_target + direction &&
          curr_source == curr_target + direction) {
        shift_count++;
        last_target = curr_target;
      } else {
        break;
      }
    }

    // Require at least 3 mov operations
    if (shift_count >= 3) {
      int64_t last_source = last_target + direction;
      if (direction == 1) {  // left shift
        int64_t start_cell = first_target;
        int64_t end_cell = last_target;
        int64_t length = shift_count;
        p.ops[i] = Operation(Operation::Type::ROL,
                             Operand(Operand::Type::DIRECT, start_cell),
                             Operand(Operand::Type::CONSTANT, length));
        p.ops.erase(p.ops.begin() + i + 1, p.ops.begin() + i + shift_count);
        p.ops.insert(p.ops.begin() + i + 1, directMov(end_cell, last_source));
      } else {  // right shift
        int64_t start_cell = last_source;
        int64_t length = shift_count + 1;
        int64_t largest_used = 0;
        if (!ProgramUtil::getUsedMemoryCells(p, nullptr, nullptr, largest_used,
                                             settings.max_memory)) {
          continue;
        }
        int64_t temp_cell = largest_used + 1;
        p.ops[i] = directMov(temp_cell, start_cell);
        p.ops.erase(p.ops.begin() + i + 1, p.ops.begin() + i + shift_count);
        p.ops.insert(p.ops.begin() + i + 1,
                     Operation(Operation::Type::ROR,
                               Operand(Operand::Type::DIRECT, start_cell),
                               Operand(Operand::Type::CONSTANT, length)));
        p.ops.insert(p.ops.begin() + i + 2, directMov(start_cell, temp_cell));
      }
      changed = true;
    }
  }
  return changed;
}

bool Optimizer::removeCommutativeDetour(Program &p) const {
  // see test E042
  if (ProgramUtil::hasIndirectOperand(p)) {
    return false;
  }
  int64_t open_loops = 0;
  for (size_t i = 0; i + 2 < p.ops.size(); i++) {
    const auto &op1 = p.ops[i];
    auto &op2 = p.ops[i + 1];
    const auto &op3 = p.ops[i + 2];
    // keep track of loops
    if (op1.type == Operation::Type::LPB) {
      open_loops++;
    } else if (op1.type == Operation::Type::LPE) {
      open_loops--;
    }
    if (open_loops > 0) {
      continue;
    }
    // check operation types
    if (op1.type != Operation::Type::MOV || op3.type != Operation::Type::MOV ||
        !ProgramUtil::isCommutative(op2.type)) {
      continue;
    }
    // check operands
    if (op1.target != op2.target || op1.target != op3.source ||
        op2.source != op3.target) {
      continue;
    }
    // check whether it is the output cell
    const auto detour_cell = op1.target;
    if (detour_cell.value == Number(Program::OUTPUT_CELL)) {
      continue;
    }
    // check whether the cell used in the detour is read later
    bool is_read = false;
    for (size_t j = i + 3; j < p.ops.size(); j++) {
      auto meta = Operation::Metadata::get(p.ops[j].type);
      if ((meta.num_operands == 2 && p.ops[j].source == detour_cell) ||
          (meta.num_operands > 0 && meta.is_reading_target &&
           p.ops[j].target == detour_cell)) {
        is_read = true;
        break;
      }
    }
    if (is_read) {
      continue;
    }
    // ok, apply change
    op2.target = op2.source;
    op2.source = op1.source;
    p.ops.erase(p.ops.begin() + i + 2);
    p.ops.erase(p.ops.begin() + i);
    return true;
  }
  return false;
}

// === OperationMover ====================================

void Optimizer::OperationMover::init(Program &p) {
  prog = &p;
  opScores.resize(p.ops.size());
  std::fill(opScores.begin(), opScores.end(), 0);
  totalScore = 0;
  for (size_t i = 0; i + 1 < p.ops.size(); i++) {
    updateScore(i);
  }
}

void Optimizer::OperationMover::updateScore(size_t i) {
  const auto score = scoreNeighbors(i);
  totalScore += score - opScores[i];
  opScores[i] = score;
}

void Optimizer::OperationMover::updateNeighborhood(size_t i) {
  const auto s = prog->ops.size();
  if (i > 0) {
    updateScore(i - 1);
  }
  if (i + 1 < s) {
    updateScore(i);
  }
  if (i + 2 < s) {
    updateScore(i + 1);
  }
}

bool Optimizer::OperationMover::up(size_t i) {
  if (i == 0 || !ProgramUtil::areIndependent(prog->ops[i - 1], prog->ops[i])) {
    return false;
  }
  std::swap(prog->ops[i - 1], prog->ops[i]);
  updateNeighborhood(i - 1);
  return true;
}

bool Optimizer::OperationMover::down(size_t i) {
  if (i + 1 == prog->ops.size() ||
      !ProgramUtil::areIndependent(prog->ops[i], prog->ops[i + 1])) {
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
    score += 40;
    if (op1.source.type == op2.source.type) {
      score += 20;
      if (canMerge(op1.type, op2.type)) {
        score += 10;
      }
    }
  } else if (op1.target.value < op2.target.value) {
    score += 1;
  }
  return score;
}

int64_t Optimizer::OperationMover::getTotalScore() const { return totalScore; }
