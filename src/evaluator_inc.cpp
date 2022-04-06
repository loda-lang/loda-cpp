#include "evaluator_inc.hpp"

#include "program_util.hpp"
#include "semantics.hpp"
#include "util.hpp"

IncrementalEvaluator::IncrementalEvaluator(Interpreter& interpreter)
    : interpreter(interpreter) {
  reset();
}

void IncrementalEvaluator::reset() {
  // program fragments and metadata
  pre_loop.ops.clear();
  loop_body.ops.clear();
  post_loop.ops.clear();
  output_updates.clear();
  output_cells.clear();
  stateful_cells.clear();
  loop_counter_dependent_cells.clear();
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
  // now the output cells are initialized
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
    if (op.type == Operation::Type::CLR || op.type == Operation::Type::DBG ||
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
      // setting, adding, subtracting constants is fine
      case Operation::Type::MOV:
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
  // we currently allow only $0 as loop counter cell
  if (loop_counter_cell != Program::INPUT_CELL) {
    return false;
  }
  return true;
}

bool IncrementalEvaluator::checkLoopBody() {
  bool loop_counter_updated = false;
  bool has_seq = false;
  bool is_commutative = true;
  for (auto& op : loop_body.ops) {
    if (op.type == Operation::Type::SEQ) {
      has_seq = true;
    }
    const auto target = op.target.value.asInt();

    // check output cells
    if (output_cells.find(target) != output_cells.end()) {
      // must be a commutative operation
      if (!ProgramUtil::isCommutative(op.type)) {
        is_commutative = false;
      }
      // cannot mix operations per memory cell (would not be commutative)
      if (output_updates.find(target) != output_updates.end() &&
          output_updates[target] != op.type) {
        is_commutative = false;
      }
      output_updates[target] = op.type;
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
      if (loop_counter_updated) {
        return false;
      }
      loop_counter_updated = true;
    }
  }
  if (!loop_counter_updated) {
    return false;
  }

  // compute set of stateful memory cells
  while (updateStatefulCells()) {
  };

  // compute set of loop counter dependent cells
  while (updateLoopCounterDependentCells()) {
  };

  // if there is more than one stateful cell or non-commutative
  // operations, it is not just a simple aggregation. therefore
  // we must ensure that there are no sequence
  // calls or loop counter dependent cells in that case.
  if ((stateful_cells.size() > 1 || !is_commutative) &&
      (has_seq || !loop_counter_dependent_cells.empty() ||
       output_cells.find(Program::INPUT_CELL) != output_cells.end())) {
    return false;
  }
  return true;
}

bool IncrementalEvaluator::updateStatefulCells() {
  bool changed = false;
  std::set<int64_t> reset;
  for (auto& op : loop_body.ops) {
    const auto meta = Operation::Metadata::get(op.type);
    if (meta.num_operands == 0) {
      continue;
    }
    const auto target = op.target.value.asInt();
    if (stateful_cells.find(target) != stateful_cells.end()) {
      continue;
    }
    if (!meta.is_writing_target) {
      continue;
    }
    if (target == loop_counter_cell) {
      continue;
    }
    if (meta.is_reading_target) {
      if (reset.find(target) == reset.end()) {
        stateful_cells.insert(target);
        changed = true;
      }
    } else {
      reset.insert(target);
    }
    if (meta.num_operands == 2 && op.source.type == Operand::Type::DIRECT &&
        stateful_cells.find(op.source.value.asInt()) != stateful_cells.end()) {
      stateful_cells.insert(target);
      changed = true;
    }
  }
  return changed;
}

bool IncrementalEvaluator::updateLoopCounterDependentCells() {
  bool changed = false;
  for (auto& op : loop_body.ops) {
    const auto meta = Operation::Metadata::get(op.type);
    const auto target = op.target.value.asInt();
    if (loop_counter_dependent_cells.find(target) !=
        loop_counter_dependent_cells.end()) {
      continue;
    }
    if (!meta.is_writing_target) {
      continue;
    }
    if (target == loop_counter_cell) {
      continue;
    }
    if (meta.num_operands == 2 && op.source.type == Operand::Type::DIRECT) {
      const auto source = op.source.value.asInt();
      const bool is_dependent = loop_counter_dependent_cells.find(source) !=
                                loop_counter_dependent_cells.end();
      // add source if it is the loop counter or dependent on it
      if (source == loop_counter_cell || is_dependent) {
        loop_counter_dependent_cells.insert(target);
        changed = true;
      }
    }
  }
  return changed;
}

bool IncrementalEvaluator::checkPostLoop() {
  // initialize output cells. all memory cells that are read
  // by the post-loop fragment are output cells.
  bool is_overwriting_output = false;
  for (auto& op : post_loop.ops) {
    const auto meta = Operation::Metadata::get(op.type);
    if (meta.num_operands > 0) {
      if (meta.is_reading_target) {
        output_cells.insert(op.target.value.asInt());
      } else if (meta.is_writing_target &&
                 op.target.value == Program::OUTPUT_CELL) {
        is_overwriting_output = true;
      }
    }
    if (meta.num_operands == 2 && op.source.type == Operand::Type::DIRECT) {
      output_cells.insert(op.source.value.asInt());
    }
  }
  if (!is_overwriting_output) {
    output_cells.insert(Program::OUTPUT_CELL);
  }
  return true;
}

// ====== Runtime of incremental evaluation ========

std::pair<Number, size_t> IncrementalEvaluator::next() {
  // sanity check: must be initialized
  if (!initialized) {
    throw std::runtime_error("incremental evaluator not initialized");
  }

  // execute pre-loop code
  tmp_state.clear();
  tmp_state.set(0, argument);
  size_t steps = interpreter.run(pre_loop, tmp_state);

  // calculate new loop count
  const int64_t new_loop_count =
      Semantics::max(tmp_state.get(loop_counter_cell), Number::ZERO).asInt();
  int64_t additional_loops = new_loop_count - previous_loop_count;
  previous_loop_count = new_loop_count;

  // update loop state
  if (argument == 0) {
    loop_state = tmp_state;
  } else {
    loop_state.set(loop_counter_cell, new_loop_count);
  }

  // execute loop body
  while (additional_loops-- > 0) {
    total_loop_steps +=
        interpreter.run(loop_body, loop_state) + 1;  // +1 for lpe
  }

  // one more iteration is needed for the correct step count
  if (argument == 0) {
    tmp_state = loop_state;
    total_loop_steps +=
        interpreter.run(loop_body, tmp_state) + 2;  // +2 for lpb  lpe
  }

  // update steps count
  steps += total_loop_steps;

  // execute post-loop code
  tmp_state = loop_state;
  steps += interpreter.run(post_loop, tmp_state);

  // prepare next iteration
  argument++;

  // return result of execution and steps
  return std::pair<Number, size_t>(tmp_state.get(0), steps);
}
