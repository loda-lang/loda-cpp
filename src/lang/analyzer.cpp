#include "lang/analyzer.hpp"

#include <algorithm>

#include "lang/program_util.hpp"

SimpleLoopProgram Analyzer::extractSimpleLoop(const Program& program) {
  // split the program into three parts:
  // 1) pre-loop
  // 2) loop body
  // 3) post-loop
  // return false if the program does not have this structure
  SimpleLoopProgram result;
  int64_t phase = 0;
  for (auto& op : program.ops) {
    if (op.type == Operation::Type::NOP) {
      continue;
    }
    if (ProgramUtil::hasIndirectOperand(op)) {
      result.is_simple_loop = false;
      return result;
    }
    if (op.type == Operation::Type::LPB) {
      if (phase != 0 || op.target.type != Operand::Type::DIRECT ||
          op.source != Operand(Operand::Type::CONSTANT, 1)) {
        result.is_simple_loop = false;
        return result;
      }
      result.counter = op.target.value.asInt();
      phase = 1;
      continue;
    }
    if (op.type == Operation::Type::LPE) {
      if (phase != 1) {
        result.is_simple_loop = false;
        return result;
      }
      phase = 2;
      continue;
    }
    if (phase == 0) {
      result.pre_loop.ops.push_back(op);
    } else if (phase == 1) {
      result.body.ops.push_back(op);
    } else if (phase == 2) {
      result.post_loop.ops.push_back(op);
    }
  }
  // need to be in the post-loop phase here for success
  result.is_simple_loop = (phase == 2);
  return result;
}

bool Analyzer::hasLogarithmicComplexity(const Program& program) {
  // check for forbidded operations
  if (ProgramUtil::numOps(program, Operation::Type::SEQ) > 0) {
    return false;
  }
  // split up the program into fragments
  auto simple_loop = extractSimpleLoop(program);
  if (!simple_loop.is_simple_loop) {
    return false;
  }
  // check for exponential growth in pre-loop fragment
  if (std::any_of(simple_loop.pre_loop.ops.begin(),
                  simple_loop.pre_loop.ops.end(), [](const Operation& op) {
                    return op.type == Operation::Type::POW &&
                           op.source.type != Operand::Type::CONSTANT;
                  })) {
    return false;
  }

  // check updates of loop counter cell in loop body
  bool loop_counter_updated = false;
  for (auto& op : simple_loop.body.ops) {
    const auto target = op.target.value.asInt();
    if (target == simple_loop.counter) {
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
