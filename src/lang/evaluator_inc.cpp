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
  simple_loop = {};
  pre_loop_filtered.ops.clear();
  output_cells.clear();
  stateful_cells.clear();
  input_dependent_cells.clear();
  loop_counter_dependent_cells.clear();
  loop_counter_decrement = 0;
  loop_counter_type = Operation::Type::NOP;
  initialized = false;

  // runtime data
  argument = 0;
  tmp_state.clear();
  loop_states.clear();
  previous_loop_counts.clear();
  total_loop_steps.clear();
  previous_slice = 0;
}

// ====== Initialization functions (static code analysis) =========

bool IncrementalEvaluator::init(const Program& program,
                                bool skip_input_transform) {
  reset();
  simple_loop = Analyzer::extractSimpleLoop(program);
  if (!simple_loop.is_simple_loop) {
    Log::get().debug("[IE] simple loop check failed");
    return false;
  }
  // now the program fragments and the loop counter cell are initialized
  if (!checkPreLoop(skip_input_transform)) {
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

bool IncrementalEvaluator::isInputDependent(const Operand& op) const {
  return (op.type == Operand::Type::DIRECT &&
          input_dependent_cells.find(op.value.asInt()) !=
              input_dependent_cells.end());
}

bool IncrementalEvaluator::checkPreLoop(bool skip_input_transform) {
  // here we do a static code analysis of the pre-loop
  // fragment to make sure here that the loop counter cell
  // is monotonically increasing (not strictly)
  pre_loop_filtered.ops.clear();
  input_dependent_cells.clear();
  input_dependent_cells.insert(Program::INPUT_CELL);
  for (auto& op : simple_loop.pre_loop.ops) {
    bool is_transform = false;
    switch (op.type) {
      case Operation::Type::MOV:
        if (isInputDependent(op.source)) {
          input_dependent_cells.insert(op.target.value.asInt());
        } else if (isInputDependent(op.target) &&
                   op.source.type == Operand::Type::CONSTANT) {
          input_dependent_cells.erase(op.target.value.asInt());
        }
        break;

      // adding, subtracting constants is fine
      case Operation::Type::ADD:
      case Operation::Type::SUB:
      case Operation::Type::TRN:
        if (op.source.type != Operand::Type::CONSTANT) {
          return false;
        }
        is_transform = true;
        break;

      // multiplying, dividing by non-negative constants is ok
      case Operation::Type::MUL:
      case Operation::Type::DIV:
      case Operation::Type::POW:
        if (op.source.type != Operand::Type::CONSTANT ||
            op.source.value < Number::ONE) {
          return false;
        }
        is_transform = true;
        break;

      default:
        // everything else is currently not allowed
        return false;
    }
    if (!skip_input_transform || !is_transform) {
      pre_loop_filtered.ops.push_back(op);
    }
  }
  // we allow only one input-dependent cell (the loop counter)
  if (input_dependent_cells.size() != 1) {
    return false;
  }
  // check if loop counter is initialized
  if (input_dependent_cells.find(simple_loop.counter) ==
      input_dependent_cells.end()) {
    return false;
  }
  return true;
}

bool IncrementalEvaluator::checkLoopBody() {
  // check loop counter cell
  bool loop_counter_updated = false;
  for (auto& op : simple_loop.body.ops) {
    const auto target = op.target.value.asInt();
    if (target == simple_loop.counter) {
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
      ProgramUtil::isCommutative(simple_loop.body, stateful_cells) &&
      ProgramUtil::isCommutative(simple_loop.body, output_cells);

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
  for (auto& op : simple_loop.body.ops) {
    const auto meta = Operation::Metadata::get(op.type);
    if (meta.num_operands == 0) {
      continue;
    }
    const auto target = op.target.value.asInt();
    if (target == simple_loop.counter) {
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
    for (auto& op : simple_loop.body.ops) {
      const auto meta = Operation::Metadata::get(op.type);
      const auto target = op.target.value.asInt();
      if (loop_counter_dependent_cells.find(target) !=
          loop_counter_dependent_cells.end()) {
        continue;
      }
      if (!meta.is_writing_target) {
        continue;
      }
      if (target == simple_loop.counter) {
        continue;
      }
      if (meta.num_operands == 2 && op.source.type == Operand::Type::DIRECT) {
        const auto source = op.source.value.asInt();
        const bool is_dependent = loop_counter_dependent_cells.find(source) !=
                                  loop_counter_dependent_cells.end();
        // add source if it is the loop counter or dependent on it
        if (source == simple_loop.counter || is_dependent) {
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
  for (auto& op : simple_loop.post_loop.ops) {
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
  previous_slice = 0;
}

std::pair<Number, size_t> IncrementalEvaluator::next(bool skip_final_iter,
                                                     bool skip_post_loop) {
  // sanity check: must be initialized
  if (!initialized) {
    throw std::runtime_error("incremental evaluator not initialized");
  }

  // execute pre-loop code
  tmp_state.clear();
  tmp_state.set(Program::INPUT_CELL, argument);
  size_t steps = interpreter.run(pre_loop_filtered, tmp_state);

  // derive loop count and slice
  const int64_t loop_counter_before =
      tmp_state.get(simple_loop.counter).asInt();
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
    loop_states[slice].set(simple_loop.counter, loop_counter_before);
  }

  // update previous loop count
  previous_loop_counts[slice] = new_loop_count;

  // execute loop body
  while (additional_loops-- > 0) {
    auto body_steps = interpreter.run(simple_loop.body, loop_states[slice]);
    total_loop_steps[slice] += body_steps + 1;  // +1 for lpb
  }

  // update steps count
  steps += total_loop_steps[slice] + 1;  // +1 for lpb of zero-th iteration

  // determine final loop counter value
  const Number final_counter_value = Semantics::min(
      Number(loop_counter_before), (loop_counter_type == Operation::Type::TRN)
                                       ? Number::ZERO
                                       : Number(slice));

  // one more iteration is needed for the correct step count
  if (!skip_final_iter) {
    tmp_state = loop_states[slice];
    tmp_state.set(simple_loop.counter, final_counter_value);
    steps += interpreter.run(simple_loop.body, tmp_state) + 1;  // +1 for lpb
  }

  // execute post-loop code
  tmp_state = loop_states[slice];
  if (!skip_post_loop) {
    tmp_state.set(simple_loop.counter, final_counter_value);
    steps += interpreter.run(simple_loop.post_loop, tmp_state);
  }

  // check maximum number of steps
  if (steps > interpreter.getMaxCycles()) {
    throw std::runtime_error("Exceeded maximum number of steps (" +
                             std::to_string(interpreter.getMaxCycles()) + ")");
  }

  // prepare next iteration
  argument++;
  previous_slice = slice;

  // return result of execution and steps
  return std::pair<Number, size_t>(tmp_state.get(Program::OUTPUT_CELL), steps);
}
