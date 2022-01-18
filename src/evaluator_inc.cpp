#include "evaluator_inc.hpp"

#include "program_util.hpp"
#include "semantics.hpp"
#include "util.hpp"

/*
; A079309: a(n) = C(1,1) + C(3,2) + C(5,3) + ... + C(2*n-1,n).
;
1,4,14,49,175,637,2353,8788,33098,125476,478192,1830270,7030570,27088870,104647630,405187825,1571990935,6109558585,23782190485,92705454895,361834392115,1413883873975,5530599237775,21654401079325,84859704298201,332818970772253,1306288683596309,5130633983976529,20164267233747049,79296558016177761,312010734643808305,1228322805115103572,4838037022123236442,19064557759743524812,75157696668074947528,296413966806493337130,1169479248974306442046,4615789573320937119346,18224297007920453127146

; PRE_LOOP
add $0,1
mov $1,2

lpb $0

  ; LOOP_BODY
  mov $2,$0
  sub $0,1
  add $2,$0
  bin $2,$0
  add $1,$2

lpe

; POST_LOOP
sub $1,2
mov $0,$1

===========================================

mov $2,$0
add $2,1
mov $3,$0

lpb $2

  mov $0,$3
  sub $2,1
  sub $0,$2
  add $4,1
  mov $5,$0
  add $5,$0
  bin $5,$0
  div $5,$4
  add $1,$5

lpe

mov $0,$1


*/

IncrementalEvaluator::IncrementalEvaluator(Interpreter& interpreter)
    : interpreter(interpreter) {
  reset();
}

void IncrementalEvaluator::reset() {
  // program fragments and metadata
  pre_loop.ops.clear();
  loop_body.ops.clear();
  post_loop.ops.clear();
  aggregation_cells.clear();
  loop_counter_cell = 0;
  initialized = false;

  // runtime data
  argument = 0;
  previous_loop_count = 0;
  total_loop_steps = 0;
  tmp_state.clear();
  loop_state.clear();
}

// ====== Initialization functions (static code analysis) =========

bool IncrementalEvaluator::init(const Program& program) {
  reset();
  if (!extractFragments(program)) {
    return false;
  }
  // now the program fragments and the loop counter cell are initialized
  if (!checkPreLoop()) {
    return false;
  }
  if (!checkPostLoop()) {
    return false;
  }
  // now the aggregation cells are initialized
  if (!checkLoopBody()) {
    return false;
  }
  initialized = true;
  return true;
}

bool IncrementalEvaluator::extractFragments(const Program& program) {
  // split the program into three parts:
  // 1) pre-loop
  // 2) loop body
  // 3) post-loop
  // return false if the program does not have this structure
  int64_t phase = 0;
  for (auto& op : program.ops) {
    if (op.type == Operation::Type::NOP) {
      continue;
    }
    if (op.type == Operation::Type::CLR ||
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
    } else if (phase == 2) {
      post_loop.ops.push_back(op);
    }
  }
  // need to be in the post-loop phase here for success
  return (phase == 2);
}

bool IncrementalEvaluator::checkPreLoop() {
  // here we do a static code analysis of the pre-loop
  // fragment to make sure here that the loop counter cell
  // is monotonically increasing (not strictly)
  for (auto& op : pre_loop.ops) {
    switch (op.type) {
      // assigning is okay
      case Operation::Type::MOV:
        break;

      // adding, subtracting constants is fine
      case Operation::Type::ADD:
      case Operation::Type::SUB:
      case Operation::Type::TRN:
        if (op.source.type != Operand::Type::CONSTANT) {
          return false;
        }
        break;

      // multiplying, dividing by non-negative constants is ok
      case Operation::Type::MUL:
      case Operation::Type::DIV:
        if (op.source.type != Operand::Type::CONSTANT ||
            op.source.value < Number::ZERO) {
          return false;
        }
        break;

      default:
        // everything else is currently not allowed
        return false;
    }
  }
  return true;
}

bool IncrementalEvaluator::checkLoopBody() {
  for (auto& op : loop_body.ops) {
    if (Operation::Metadata::get(op.type).num_operands == 0) {
      continue;
    }
    const auto target = op.target.value.asInt();
    // check aggregation cells
    if (aggregation_cells.find(target) != aggregation_cells.end()) {
      // must be a commutative operation
      if (op.type != Operation::Type::ADD && op.type != Operation::Type::MUL) {
        return false;
      }
    }
    // check loop counter cell
    if (target == loop_counter_cell) {
      // must be subtraction by one (stepwise decrease)
      if (op.type != Operation::Type::SUB && op.type != Operation::Type::TRN) {
        return false;
      }
      if (op.source != Operand(Operand::Type::CONSTANT, Number::ONE)) {
        return false;
      }
    }
  }
  return true;
}

bool IncrementalEvaluator::checkPostLoop() {
  // initialize aggregation cells. all memory cells that are read
  // by the post-loop fragment must be agregation cells
  bool is_overwriting_output = false;
  for (auto& op : post_loop.ops) {
    const auto meta = Operation::Metadata::get(op.type);
    if (meta.num_operands > 0) {
      if (meta.is_reading_target) {
        aggregation_cells.insert(op.target.value.asInt());
      } else if (meta.is_writing_target &&
                 op.target.value == Program::OUTPUT_CELL) {
        is_overwriting_output = true;
      }
    }
    if (meta.num_operands > 1 && op.source.type == Operand::Type::DIRECT) {
      aggregation_cells.insert(op.source.value.asInt());
    }
  }
  if (!is_overwriting_output) {
    aggregation_cells.insert(Program::OUTPUT_CELL);
  }
  return true;
}

// ====== Runtime of incremental evaluation ========

std::pair<Number, size_t> IncrementalEvaluator::next() {
  // sanity check: must be initialized
  if (!initialized) {
    throw std::runtime_error("incremental evaluator not initialized");
  }
  // std::cout << "\n==== EVAL N=" << argument << std::endl << std::endl;

  // execute pre-loop code
  tmp_state.clear();
  tmp_state.set(0, argument);
  size_t steps = runFragment(pre_loop, tmp_state);

  // calculate new loop count
  const int64_t new_loop_count = tmp_state.get(loop_counter_cell).asInt();
  int64_t additional_loops = new_loop_count - previous_loop_count;
  previous_loop_count = new_loop_count;

  // sanity check: loop count cannot be negative
  if (additional_loops < 0) {
    throw std::runtime_error("unexpected loop count: " +
                             std::to_string(additional_loops));
  }

  // update loop state
  if (argument == 0) {
    loop_state = tmp_state;
  } else {
    loop_state.set(0, new_loop_count);
  }

  // execute loop body
  while (additional_loops-- > 0) {
    total_loop_steps += runFragment(loop_body, loop_state) + 1;  // +1 for lpe
  }

  // one more iteration is needed for the correct step count
  if (argument == 0) {
    tmp_state = loop_state;
    total_loop_steps +=
        runFragment(loop_body, tmp_state) + 2;  // +2 for lpb  lpe
  }

  // update steps count
  steps += total_loop_steps;

  // execute post-loop code
  tmp_state = loop_state;
  steps += runFragment(post_loop, tmp_state);

  // prepare next iteration
  argument++;

  // return result of execution and steps
  return std::pair<Number, size_t>(tmp_state.get(0), steps);
}

size_t IncrementalEvaluator::runFragment(const Program& p, Memory& state) {
  // std::cout << "CURRENT MEMORY: " << state << std::endl;
  // ProgramUtil::print(p, std::cout);
  return interpreter.run(p, state);
}
