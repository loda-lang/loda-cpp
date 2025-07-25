#include "eval/minimizer.hpp"

#include <fstream>
#include <set>

#include "eval/optimizer.hpp"
#include "eval/semantics.hpp"
#include "lang/constants.hpp"
#include "lang/program_util.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"
#include "sys/util.hpp"

bool Minimizer::minimize(Program& p, size_t num_terms) const {
  Log::get().debug("Minimizing program");
  evaluator.clearCaches();

  // calculate target sequence
  Sequence target_sequence;
  steps_t target_steps = evaluator.eval(p, target_sequence, num_terms, false);
  if (Signals::HALT) {
    return false;  // interrupted evaluation
  }

  if (target_sequence.size() < settings.num_terms) {
    Log::get().error(
        "Cannot minimize program because there are too few terms: " +
            std::to_string(target_sequence.size()),
        false);
    return false;
  }

  bool global_change = false;

  // replace "clr" operations
  if (replaceClr(p)) {
    global_change = true;
  }

  // replace constant loops
  for (int64_t exp = 1; exp <= 5; exp++) {
    if (replaceConstantLoop(p, target_sequence, exp)) {
      global_change = true;
      break;
    }
  }

  // remove or replace operations
  for (int64_t i = 0; i < (int64_t)p.ops.size(); ++i) {
    bool local_change = false;
    const auto op = p.ops[i];  // make a backup of the original operation
    if (op.type == Operation::Type::LPE) {
      continue;
    } else if (op.type == Operation::Type::TRN) {
      p.ops[i].type = Operation::Type::SUB;
      if (check(p, target_sequence, target_steps.total)) {
        local_change = true;
      } else {
        // revert change
        p.ops[i] = op;
      }
    } else if (op.type == Operation::Type::LPB) {
      if (op.source.type != Operand::Type::CONSTANT || op.source.value != 1) {
        p.ops[i].source = Operand(Operand::Type::CONSTANT, 1);
        if (check(p, target_sequence, target_steps.total)) {
          local_change = true;
        } else {
          // revert change
          p.ops[i] = op;
        }
      }
    } else if (p.ops.size() > 1) {
      // try to remove the current operation (if there is at least one
      // operation, see A000004)
      p.ops.erase(p.ops.begin() + i, p.ops.begin() + i + 1);
      if (check(p, target_sequence, target_steps.total)) {
        local_change = true;
        --i;
      } else {
        // revert change
        p.ops.insert(p.ops.begin() + i, op);
      }
    }

    if (!local_change) {
      // gcd with larger power of small constant? => replace with a loop
      if (op.type == Operation::Type::GCD &&
          op.target.type == Operand::Type::DIRECT &&
          op.source.type == Operand::Type::CONSTANT &&
          op.source.value != Number::ZERO) {
        int64_t base = getPowerOf(op.source.value);
        if (base != 0) {
          std::unordered_set<int64_t> used_cells;
          int64_t largest_used = 0;
          if (ProgramUtil::getUsedMemoryCells(p, used_cells, largest_used,
                                              settings.max_memory)) {
            // try to replace gcd by a loop
            auto tmp = Operand(Operand::Type::DIRECT, largest_used + 1);
            p.ops[i] = Operation(Operation::Type::MOV, tmp,
                                 Operand(Operand::Type::CONSTANT, 1));
            p.ops.insert(p.ops.begin() + i + 1,
                         Operation(Operation::Type::LPB, op.target,
                                   Operand(Operand::Type::CONSTANT, 1)));
            p.ops.insert(p.ops.begin() + i + 2,
                         Operation(Operation::Type::MUL, tmp,
                                   Operand(Operand::Type::CONSTANT, base)));
            p.ops.insert(p.ops.begin() + i + 3,
                         Operation(Operation::Type::DIF, op.target,
                                   Operand(Operand::Type::CONSTANT, base)));
            p.ops.insert(p.ops.begin() + i + 4,
                         Operation(Operation::Type::LPE));
            p.ops.insert(p.ops.begin() + i + 5,
                         Operation(Operation::Type::MOV, op.target, tmp));

            // we don't check the number of steps here!
            if (check(p, target_sequence, 0)) {
              local_change = true;
            } else {
              // revert change
              p.ops[i] = op;
              p.ops.erase(p.ops.begin() + i + 1, p.ops.begin() + i + 6);
            }
          }
        }
      }
    }
    global_change = global_change || local_change;
  }
  return global_change;
}

bool Minimizer::check(const Program& p, const Sequence& seq,
                      size_t max_total) const {
  try {
    auto res = evaluator.check(p, seq);
    if (res.first != status_t::OK) {
      return false;
    }
    if (max_total > 0 && res.second.total > max_total) {
      return false;
    }
  } catch (const std::exception&) {
    return false;
  }
  return true;
}

int64_t Minimizer::getPowerOf(const Number& v) {
  std::vector<int64_t> bases = {2, 3, 5, 7, 10};         // magic number
  std::vector<int64_t> min_exponents = {7, 6, 5, 5, 4};  // magic number
  for (size_t i = 0; i < bases.size(); i++) {
    auto base = bases.at(i);
    auto exponent = Semantics::getPowerOf(v, base);
    if (exponent == Number::INF) {
      continue;
    }
    if (min_exponents.at(i) <= exponent.asInt()) {
      return base;
    }
  }
  return 0;
}

bool Minimizer::replaceClr(Program& p) const {
  bool replaced = false;
  for (size_t i = 0; i < p.ops.size(); i++) {
    auto& op = p.ops[i];
    if (op.type == Operation::Type::CLR &&
        op.target.type == Operand::Type::DIRECT &&
        op.source.type == Operand::Type::CONSTANT) {
      const int64_t length = op.source.value.asInt();
      if (length > 0 && length <= 100) {  // magic number
        op.type = Operation::Type::MOV;
        op.source.value = 0;
        auto mov = op;
        for (int64_t j = 1; j < length; j++) {
          mov.target.value = Semantics::add(mov.target.value, Number::ONE);
          p.ops.insert(p.ops.begin() + i + j, mov);
        }
        replaced = true;
      }
    }
  }
  return replaced;
}

bool Minimizer::replaceConstantLoop(Program& p, const Sequence& seq,
                                    int64_t exp) const {
  // check pre-conditions
  auto info = Constants::findConstantLoop(p);
  if (!info.has_constant_loop) {
    return false;
  }
  if (info.constant_value < Number(100)) {  // magic number
    return false;
  }
  // limitations:
  // 1) mov operation with a constant must be directly before the loop
  // 2) input cell must not be overwritten
  if (info.index_lpb == 0) {
    return false;
  }
  const Operation& old_mov = p.ops[info.index_lpb - 1];
  const Operation& lpb = p.ops[info.index_lpb];
  if (old_mov.type != Operation::Type::MOV || old_mov.target != lpb.target ||
      old_mov.source.type != Operand::Type::CONSTANT ||
      info.is_input_overwritten) {
    return false;
  }

  // apply change
  auto backup = p;
  Operation mov(Operation::Type::MOV, lpb.target,
                Operand(Operand::Type::DIRECT, Number(Program::INPUT_CELL)));
  Operation add(Operation::Type::ADD, lpb.target,
                Operand(Operand::Type::CONSTANT, Number(2 * exp)));
  Operation pow(Operation::Type::POW, lpb.target,
                Operand(Operand::Type::CONSTANT, Number(exp)));
  p.ops[info.index_lpb - 1] = mov;
  p.ops.insert(p.ops.begin() + info.index_lpb, add);
  p.ops.insert(p.ops.begin() + info.index_lpb + 1, pow);
  if (check(p, seq, 0)) {
    return true;
  } else {
    p = backup;
    return false;
  }
}

void dumpProgram(const Program& p) {
  std::string f = Setup::getLodaHome() + "debug" + FILE_SEP + "minimizer" +
                  FILE_SEP + std::to_string(ProgramUtil::hash(p) % 100000) +
                  ".asm";
  ensureDir(f);
  std::ofstream out(f);
  ProgramUtil::print(p, out);
}

bool Minimizer::optimizeAndMinimize(Program& p, size_t num_terms) const {
  Program backup = p;
  try {
    std::set<Program> stages;
    bool optimized = false, minimized = false, result = false;
    do {
      if (stages.find(p) != stages.end()) {
        Log::get().warn("Detected optimization/minimization loop");
        dumpProgram(p);
        break;
      }
      stages.insert(p);
      optimized = optimizer.optimize(p);
      minimized = minimize(p, num_terms);
      result = result || optimized || minimized;
    } while (optimized || minimized);
    return result;
  } catch (std::exception& e) {
    // revert change
    p = backup;
    // log error and dump program for later analysis
    Log::get().error("Exception during minimization: " + std::string(e.what()),
                     false);
    dumpProgram(p);
  }
  return false;
}
