#include "minimizer.hpp"

#include <fstream>

#include "file.hpp"
#include "optimizer.hpp"
#include "program_util.hpp"
#include "semantics.hpp"
#include "setup.hpp"
#include "util.hpp"

bool Minimizer::minimize(Program& p, size_t num_terms) const {
  Log::get().debug("Minimizing program");

  // calculate target sequence
  Sequence target_sequence;
  steps_t target_steps = evaluator.eval(p, target_sequence, num_terms, false);
  if (target_sequence.size() < settings.num_terms) {
    Log::get().error(
        "Cannot minimize program because there are too few terms: " +
            std::to_string(target_sequence.size()),
        false);
    return false;
  }

  // remove "clr" operations
  bool global_change = removeClr(p);

  // remove or replace operations
  bool local_change;
  for (int64_t i = 0; i < (int64_t)p.ops.size(); ++i) {
    local_change = false;
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
  if (Number(9) < Semantics::getPowerOf(v, 2)) {
    return 2;
  } else if (Number(5) < Semantics::getPowerOf(v, 3)) {
    return 3;
  } else if (Number(4) < Semantics::getPowerOf(v, 5)) {
    return 5;
  } else if (Number(3) < Semantics::getPowerOf(v, 7)) {
    return 7;
  } else if (Number(2) < Semantics::getPowerOf(v, 10)) {
    return 10;
  }
  return 0;
}

bool Minimizer::removeClr(Program& p) const {
  bool replaced = false;
  for (size_t i = 0; i < p.ops.size(); i++) {
    auto& op = p.ops[i];
    if (op.type == Operation::Type::CLR &&
        op.target.type == Operand::Type::DIRECT &&
        op.source.type == Operand::Type::CONSTANT) {
      const int64_t length = op.source.value.asInt();
      if (length <= 0) {
        p.ops.erase(p.ops.begin() + i);
        replaced = true;
      } else if (length <= 100) {  // magic number
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

bool Minimizer::optimizeAndMinimize(Program& p, size_t num_terms) const {
  Program backup = p;
  try {
    bool optimized = false, minimized = false, result = false;
    do {
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
    std::string f = Setup::getLodaHome() + "debug/minimizer/" +
                    std::to_string(ProgramUtil::hash(p) % 100000) + ".asm";
    ensureDir(f);
    std::ofstream out(f);
    ProgramUtil::print(p, out);
  }
  return false;
}
