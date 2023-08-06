#include "lang/analyzer.hpp"

#include <algorithm>

#include "lang/program_util.hpp"

SimpleLoopProgram Analyzer::extractSimpleLoop(const Program& program) {
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
  // check for forbidden operation types
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

bool isConstantGreaterOne(const Operand& op) {
  return (op.type == Operand::Type::CONSTANT && Number::ONE < op.value);
}

// ensure that the pre-loop contains exponential growth
// example pre-loop:
//   mov $1,2  ; [required,phase:=1] init loop counter with a constant >1
//   add $0,1  ; [optional] increase argument
//   pow $1,$0 ; [required,phase:=2] exponetial growth of loop counter
//   mov $2,7  ; [optional] initialize other cells
bool isExponentialPreLoop(const Program& pre_loop, int64_t counter) {
  // loop counter must be different than argument
  if (counter == Program::INPUT_CELL) {
    return false;
  }
  int64_t phase = 0;
  for (auto& op : pre_loop.ops) {
    auto target = op.target.value.asInt();
    // loop counter update
    if (target == counter) {
      // initialization of loop counter with constant >1
      if (phase == 0 && op.type == Operation::Type::MOV &&
          isConstantGreaterOne(op.source)) {
        phase = 1;
      }
      // exponential growth of loop counter
      else if (phase == 1 && op.type == Operation::Type::POW &&
               op.source ==
                   Operand(Operand::Type::DIRECT, Program::INPUT_CELL)) {
        phase = 2;
      } else {
        // everything else is not ok
        return false;
      }
    }
    // argument update
    else if (target == Program::INPUT_CELL) {
      // check for allowed updates
      if (op.type != Operation::Type::ADD && op.type != Operation::Type::MUL) {
        return false;
      }
      if (!isConstantGreaterOne(op.source)) {
        return false;
      }
      // argument update is ok
    }
    // updates to other cells are ok
  }
  // must be in the last phase
  return (phase == 2);
}

bool isLinearBody(const Program& body, int64_t counter) {
  // check updates of loop counter cell in loop body
  bool loop_counter_updated = false;
  for (auto& op : body.ops) {
    const auto target = op.target.value.asInt();
    if (target == counter) {
      loop_counter_updated = true;
      // loop counter must be updated using subtraction or truncation
      if (op.type != Operation::Type::SUB && op.type != Operation::Type::TRN) {
        return false;
      }
      // all updates must be using a positive constant argument
      if (!isConstantGreaterOne(op.source)) {
        return false;
      }
    }
  }
  return loop_counter_updated;
}

bool Analyzer::hasExponentialComplexity(const Program& program) {
  // split up the program into fragments
  auto simple_loop = extractSimpleLoop(program);
  if (!simple_loop.is_simple_loop) {
    return false;
  }
  // check pre-loop
  if (!isExponentialPreLoop(simple_loop.pre_loop, simple_loop.counter)) {
    return false;
  }
  // check body
  if (!isLinearBody(simple_loop.body, simple_loop.counter)) {
    return false;
  }
  // success: program has exponential complexity
  return true;
}
