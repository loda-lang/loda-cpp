#include "lang/evaluator_inc.hpp"

#include "lang/program_util.hpp"
#include "lang/semantics.hpp"
#include "sys/log.hpp"
#include "sys/util.hpp"

IncrementalEvaluator::IncrementalEvaluator(Interpreter& interpreter)
    : interpreter(interpreter) {
  reset();
}

void IncrementalEvaluator::reset() {
  // program fragments and metadata
  pre_loop.ops.clear();
  loop_body.ops.clear();
  post_loop.ops.clear();
  output_cells.clear();
  stateful_cells.clear();
  loop_counter_dependent_cells.clear();
  loop_counter_cell = 0;
  loop_counter_decrement = 0;
  loop_counter_type = Operation::Type::NOP;
  initialized = false;

  // runtime data
  argument = 0;
  tmp_state.clear();
  loop_states.clear();
  previous_loop_counts.clear();
  total_loop_steps.clear();
}

// ====== Initialization functions (static code analysis) =========

bool IncrementalEvaluator::init(const Program& program) {
  reset();
  if (!extractFragments(program)) {
    Log::get().debug("[IE] extraction of fragments failed");
    return false;
  }
  // now the program fragments and the loop counter cell are initialized
  if (!checkPreLoop()) {
    Log::get().debug("[IE] pre-loop check failed");
    return false;
  }
  if (!checkPostLoop()) {
    Log::get().debug("[IE] post-loop check failed");
    return false;
  }
  // now the output cells are initialized
  if (!checkLoopBody()) {
    Log::get().debug("[IE] loop body check failed");
    return false;
  }
  initRuntimeData();
  initialized = true;
  Log::get().debug("[IE] initialization successful");
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
    if (op.type == Operation::Type::DBG ||
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
  static const Operand input_op(Operand::Type::DIRECT, Program::INPUT_CELL);
  bool loop_counter_initialized = (loop_counter_cell == Program::INPUT_CELL);
  bool needs_input_reset = false;
  for (auto& op : pre_loop.ops) {
    switch (op.type) {
      case Operation::Type::MOV:
        // using other cells as loop counters is allowed
        if (op.target.value.asInt() == loop_counter_cell) {
          if (op.source != input_op) {
            return false;
          }
          loop_counter_initialized = true;
          needs_input_reset = true;
        } else {
          // non-loop-counters can be initialized only with constants
          if (op.source.type != Operand::Type::CONSTANT) {
            return false;
          }
          if (op.target.value.asInt() == Program::INPUT_CELL) {
            needs_input_reset = false;
          }
        }
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
      case Operation::Type::POW:
        if (op.source.type != Operand::Type::CONSTANT ||
            op.source.value < Number::ONE) {
          return false;
        }
        break;

      default:
        // everything else is currently not allowed
        return false;
    }
  }
  if (!loop_counter_initialized || needs_input_reset) {
    return false;
  }
  return true;
}

bool IncrementalEvaluator::isCommutative(int64_t cell) const {
  auto update_type = Operation::Type::NOP;
  for (auto& op : loop_body.ops) {
    const auto meta = Operation::Metadata::get(op.type);
    const auto target = op.target.value.asInt();
    if (target == cell) {
      if (!ProgramUtil::isCommutative(op.type)) {
        return false;
      }
      if (update_type == Operation::Type::NOP) {
        update_type = op.type;
      } else if (update_type != op.type) {
        return false;
      }
    }
    if (meta.num_operands == 2 && op.source.type == Operand::Type::DIRECT) {
      const auto source = op.source.value.asInt();
      if (source == cell) {
        return false;
      }
    }
  }
  return true;
}

bool IncrementalEvaluator::isCommutative(const std::set<int64_t>& cells) const {
  for (auto c : cells) {
    if (!isCommutative(c)) {
      return false;
    }
  }
  return true;
}

bool IncrementalEvaluator::checkLoopBody() {
  // check loop counter cell
  bool loop_counter_updated = false;
  for (auto& op : loop_body.ops) {
    const auto target = op.target.value.asInt();
    if (target == loop_counter_cell) {
      // must be subtraction by one (stepwise decrease)
      if (op.type != Operation::Type::SUB && op.type != Operation::Type::TRN) {
        return false;
      }
      loop_counter_type = op.type;
      if (op.source.type != Operand::Type::CONSTANT) {
        return false;
      }
      if (loop_counter_updated) {
        return false;
      }
      loop_counter_updated = true;
      loop_counter_decrement = op.source.value.asInt();
    }
  }
  if (!loop_counter_updated) {
    return false;
  }
  if (loop_counter_decrement < 1 ||
      loop_counter_decrement >
          1000) {  // prevent exhaustive memory usage; magic number
    return false;
  }

  // compute set of stateful memory cells
  computeStatefulCells();

  // compute set of loop counter dependent cells
  computeLoopCounterDependentCells();

  // check if stateful cells and output cells are commutative
  bool is_commutative =
      isCommutative(stateful_cells) && isCommutative(output_cells);

  // ================================================= //
  // === from now on, we check for positive cases ==== //
  // ================================================= //

  if (loop_counter_dependent_cells.empty()) {
    return true;
  }

  if (stateful_cells.size() <= 1 && is_commutative) {
    return true;
  }

  // IE not supported
  return false;
}

void IncrementalEvaluator::computeStatefulCells() {
  std::set<int64_t> read;
  std::set<int64_t> write;
  stateful_cells.clear();
  for (auto& op : loop_body.ops) {
    const auto meta = Operation::Metadata::get(op.type);
    if (meta.num_operands == 0) {
      continue;
    }
    const auto target = op.target.value.asInt();
    if (target == loop_counter_cell) {
      continue;
    }
    // update read cells
    if (meta.is_reading_target) {
      read.insert(target);
    }
    if (meta.num_operands == 2 && op.source.type == Operand::Type::DIRECT) {
      read.insert(op.source.value.asInt());
    }
    // update written cells
    if (meta.is_writing_target && write.find(target) == write.end()) {
      if (read.find(target) != read.end()) {
        stateful_cells.insert(target);
      }
      write.insert(target);
    }
  }
}

void IncrementalEvaluator::computeLoopCounterDependentCells() {
  loop_counter_dependent_cells.clear();
  bool changed = true;
  while (changed) {
    changed = false;
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
  }
}

bool IncrementalEvaluator::checkPostLoop() {
  // initialize output cells. all memory cells that are read
  // by the post-loop fragment are output cells.
  std::set<int64_t> write;
  for (auto& op : post_loop.ops) {
    const auto meta = Operation::Metadata::get(op.type);
    if (meta.num_operands < 1) {
      continue;
    }
    auto target = op.target.value.asInt();
    if (meta.is_reading_target) {
      if (write.find(target) == write.end()) {
        output_cells.insert(target);
      }
    }
    if (meta.is_writing_target) {
      write.insert(target);
    }
    if (meta.num_operands < 2) {
      continue;
    }
    if (op.source.type != Operand::Type::DIRECT) {
      continue;
    }
    auto source = op.source.value.asInt();
    if (write.find(source) == write.end()) {
      output_cells.insert(source);
    }
  }
  if (write.find(Program::OUTPUT_CELL) == write.end()) {
    output_cells.insert(Program::OUTPUT_CELL);
  }
  return true;
}

// ====== Runtime of incremental evaluation ========

void IncrementalEvaluator::initRuntimeData() {
  loop_states.resize(loop_counter_decrement);
  previous_loop_counts.resize(loop_counter_decrement, 0);
  total_loop_steps.resize(loop_counter_decrement, 0);
}

std::pair<Number, size_t> IncrementalEvaluator::next() {
  // sanity check: must be initialized
  if (!initialized) {
    throw std::runtime_error("incremental evaluator not initialized");
  }

  // execute pre-loop code
  tmp_state.clear();
  tmp_state.set(Program::INPUT_CELL, argument);
  size_t steps = interpreter.run(pre_loop, tmp_state);

  // derive loop count and slice
  const int64_t loop_counter_before = tmp_state.get(loop_counter_cell).asInt();
  const int64_t new_loop_count = std::max<int64_t>(loop_counter_before, 0);
  const int64_t slice = new_loop_count % loop_counter_decrement;

  // calculate number of additional loops
  int64_t additional_loops =
      (new_loop_count - previous_loop_counts[slice]) / loop_counter_decrement;

  // one more iteration may be needed when using trn
  if (previous_loop_counts[slice] == 0 &&
      loop_counter_type == Operation::Type::TRN &&
      new_loop_count % loop_counter_decrement) {
    additional_loops++;
  }

  // init or update loop state
  if (previous_loop_counts[slice] == 0) {
    loop_states[slice] = tmp_state;
  } else {
    loop_states[slice].set(loop_counter_cell, loop_counter_before);
  }

  // update previous loop count
  previous_loop_counts[slice] = new_loop_count;

  // execute loop body
  while (additional_loops-- > 0) {
    total_loop_steps[slice] +=
        interpreter.run(loop_body, loop_states[slice]) + 1;  // +1 for lpb
  }

  // update steps count
  steps += total_loop_steps[slice] + 1;  // +1 for lpb of zero-th iteration

  // one more iteration is needed for the correct step count
  const Number final_counter_value = Semantics::min(
      Number(loop_counter_before), (loop_counter_type == Operation::Type::TRN)
                                       ? Number::ZERO
                                       : Number(slice));
  tmp_state = loop_states[slice];
  tmp_state.set(loop_counter_cell, final_counter_value);
  steps += interpreter.run(loop_body, tmp_state) + 1;  // +1 for lpb

  // execute post-loop code
  tmp_state = loop_states[slice];
  tmp_state.set(loop_counter_cell, final_counter_value);
  steps += interpreter.run(post_loop, tmp_state);

  // check maximum number of steps
  if (steps > interpreter.getMaxCycles()) {
    throw std::runtime_error("Exceeded maximum number of steps (" +
                             std::to_string(interpreter.getMaxCycles()) + ")");
  }

  // prepare next iteration
  argument++;

  // return result of execution and steps
  return std::pair<Number, size_t>(tmp_state.get(0), steps);
}
