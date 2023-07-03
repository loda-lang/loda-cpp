#include "lang/evaluator_log.hpp"

#include <algorithm>

#include "lang/program_util.hpp"

bool LogarithmicEvaluator::hasLogarithmicComplexity(const Program& program) {
  // split up the program into fragments:
  Program pre_loop, loop_body;
  int64_t phase = 0;
  int64_t loop_counter_cell = 0;
  for (auto& op : program.ops) {
    if (op.type == Operation::Type::NOP) {
      continue;
    }
    // check for forbidden operation/operand types
    if (op.type == Operation::Type::SEQ ||
        ProgramUtil::hasIndirectOperand(op)) {
      return false;
    }
    if (op.type == Operation::Type::LPB) {
      if (phase != 0 || op.target.type != Operand::Type::DIRECT ||
          op.source != Operand(Operand::Type::CONSTANT, 1)) {
        return false;
      }
      loop_counter_cell = op.target.value.asInt();
      phase = 1;
      continue;
    }
    if (op.type == Operation::Type::LPE) {
      if (phase != 1) {
        return false;
      }
      phase = 2;
      continue;
    }
    if (phase == 0) {
      pre_loop.ops.push_back(op);
    } else if (phase == 1) {
      loop_body.ops.push_back(op);
    }
  }

  // need to be in the post-loop phase here for success
  if (phase != 2) {
    return false;
  }

  // check for exponential growth in pre-loop fragment
  if (std::any_of(pre_loop.ops.begin(), pre_loop.ops.end(),
                  [](const Operation& op) {
                    return op.type == Operation::Type::POW &&
                           op.source.type != Operand::Type::CONSTANT;
                  })) {
    return false;
  }

  // check updates of loop counter cell in loop body
  bool loop_counter_updated = false;
  for (auto& op : loop_body.ops) {
    const auto target = op.target.value.asInt();
    if (target == loop_counter_cell) {
      // loop counter must be updated using division
      if (op.type == Operation::Type::DIV || op.type == Operation::Type::DIF) {
        loop_counter_updated = true;
      } else {
        return false;
      }
      // all updates must be using a constant argument
      if (op.source.type != Operand::Type::CONSTANT) {
        return false;
      }
    }
  }
  if (!loop_counter_updated) {
    return false;
  }

  // success: program has log complexity
  return true;
}
