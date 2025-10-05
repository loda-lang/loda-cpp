#include "eval/evaluator_inc.hpp"

#include "eval/semantics.hpp"
#include "lang/program_util.hpp"
#include "sys/log.hpp"
#include "sys/util.hpp"

IncrementalEvaluator::IncrementalEvaluator(Interpreter& interpreter)
    : interpreter(interpreter),
      is_debug(Log::get().level == Log::Level::DEBUG) {
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
  loop_counter_lower_bound = 0;
  loop_counter_type = Operation::Type::NOP;
  offset = 0;
  initialized = false;
  last_error_code = ErrorCode::OK;

  // runtime data
  argument = 0;
  tmp_state.clear();
  loop_states.clear();
  previous_loop_counts.clear();
  total_loop_steps.clear();
  previous_slice = 0;
}

// ====== Initialization functions (static code analysis) =========

SimpleLoopProgram IncrementalEvaluator::extractSimpleLoopWithError(
    const Program& program, ErrorCode* error_code) {
  SimpleLoopProgram result;
  int64_t phase = 0;
  for (auto& op : program.ops) {
    if (op.type == Operation::Type::NOP) {
      continue;
    }
    if (ProgramUtil::hasIndirectOperand(op)) {
      result.is_simple_loop = false;
      if (error_code) {
        *error_code = ErrorCode::HAS_INDIRECT_OPERAND;
      }
      return result;
    }
    if (op.type == Operation::Type::LPB) {
      if (phase != 0) {
        result.is_simple_loop = false;
        if (error_code) {
          *error_code = ErrorCode::MULTIPLE_LOOPS;
        }
        return result;
      }
      if (op.target.type != Operand::Type::DIRECT) {
        result.is_simple_loop = false;
        if (error_code) {
          *error_code = ErrorCode::LPB_TARGET_NOT_DIRECT;
        }
        return result;
      }
      if (op.source != Operand(Operand::Type::CONSTANT, 1)) {
        result.is_simple_loop = false;
        if (error_code) {
          *error_code = ErrorCode::LPB_SOURCE_NOT_ONE;
        }
        return result;
      }
      result.counter = op.target.value.asInt();
      phase = 1;
      continue;
    }
    if (op.type == Operation::Type::LPE) {
      if (phase != 1) {
        result.is_simple_loop = false;
        if (error_code) {
          *error_code = ErrorCode::LPE_WITHOUT_LPB;
        }
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
  if (!result.is_simple_loop && error_code) {
    *error_code = ErrorCode::NO_LOOP_FOUND;
  }
  return result;
}

bool IncrementalEvaluator::init(const Program& program,
                                bool skip_input_transform, bool skip_offset,
                                ErrorCode* error_code) {
  reset();
  ErrorCode local_error_code = ErrorCode::OK;
  simple_loop = extractSimpleLoopWithError(program, &local_error_code);
  if (!simple_loop.is_simple_loop) {
    last_error_code = local_error_code;
    if (error_code) {
      *error_code = local_error_code;
    }
    if (is_debug) {
      Log::get().debug("[IE] Simple loop check failed");
    }
    return false;
  }
  // now the program fragments and the loop counter cell are initialized
  if (!checkPreLoop(skip_input_transform, &local_error_code)) {
    last_error_code = local_error_code;
    if (error_code) {
      *error_code = local_error_code;
    }
    if (is_debug) {
      Log::get().debug("[IE] Pre-loop check failed");
    }
    return false;
  }
  if (!checkPostLoop(&local_error_code)) {
    last_error_code = local_error_code;
    if (error_code) {
      *error_code = local_error_code;
    }
    if (is_debug) {
      Log::get().debug("[IE] Post-loop check failed");
    }
    return false;
  }
  // now the output cells are initialized
  if (!checkLoopBody(&local_error_code)) {
    last_error_code = local_error_code;
    if (error_code) {
      *error_code = local_error_code;
    }
    if (is_debug) {
      Log::get().debug("[IE] Loop body check failed");
    }
    return false;
  }
  // extract offset from program directive
  offset = skip_offset ? 0 : ProgramUtil::getOffset(program);

  // initialue the runtime data
  initRuntimeData();
  initialized = true;
  last_error_code = ErrorCode::OK;
  if (error_code) {
    *error_code = ErrorCode::OK;
  }
  if (is_debug) {
    Log::get().debug("[IE] Initialization successful");
  }
  return true;
}

bool IncrementalEvaluator::isInputDependent(const Operand& op) const {
  return (op.type == Operand::Type::DIRECT &&
          input_dependent_cells.find(op.value.asInt()) !=
              input_dependent_cells.end());
}

bool IncrementalEvaluator::checkPreLoop(bool skip_input_transform,
                                        ErrorCode* error_code) {
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
          if (error_code) {
            *error_code = ErrorCode::PRELOOP_NON_CONSTANT_OPERAND;
          }
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
          if (error_code) {
            *error_code = ErrorCode::PRELOOP_NON_CONSTANT_OPERAND;
          }
          return false;
        }
        is_transform = true;
        break;

      default:
        // everything else is currently not allowed
        if (error_code) {
          *error_code = ErrorCode::PRELOOP_UNSUPPORTED_OPERATION;
        }
        return false;
    }
    if (!skip_input_transform || !is_transform) {
      pre_loop_filtered.ops.push_back(op);
    }
  }
  // check if loop counter is initialized
  if (input_dependent_cells.find(simple_loop.counter) ==
      input_dependent_cells.end()) {
    if (error_code) {
      *error_code = ErrorCode::LOOP_COUNTER_NOT_INPUT_DEPENDENT;
    }
    return false;
  }
  return true;
}

bool IncrementalEvaluator::checkLoopBody(ErrorCode* error_code) {
  // check loop counter cell
  bool loop_counter_updated = false;
  for (const auto& op : simple_loop.body.ops) {
    const auto& meta = Operation::Metadata::get(op.type);
    const auto target = op.target.value.asInt();
    if (target == simple_loop.counter) {
      if ((op.type == Operation::Type::SUB ||
           op.type == Operation::Type::TRN) &&
          op.source.type == Operand::Type::CONSTANT && !loop_counter_updated) {
        loop_counter_type = op.type;
        loop_counter_updated = true;
        loop_counter_decrement = op.source.value.asInt();
        loop_counter_lower_bound = std::max<int64_t>(
            loop_counter_lower_bound - loop_counter_decrement, 0);
      } else if (op.type == Operation::Type::MAX &&
                 op.source.type == Operand::Type::CONSTANT) {
        loop_counter_lower_bound = std::max<int64_t>(loop_counter_lower_bound,
                                                     op.source.value.asInt());
      } else {
        if (error_code) {
          *error_code = ErrorCode::LOOP_COUNTER_UPDATE_INVALID;
        }
        return false;
      }
    } else if (meta.num_operands > 0 && isInputDependent(op.target) &&
               meta.is_reading_target) {
      if (error_code) {
        *error_code = ErrorCode::INPUT_DEPENDENT_CELL_READ;
      }
      return false;
    } else if (meta.num_operands > 1 && isInputDependent(op.source) &&
               op.source.value.asInt() != simple_loop.counter) {
      if (error_code) {
        *error_code = ErrorCode::INPUT_DEPENDENT_SOURCE_USED;
      }
      return false;
    }
  }
  if (!loop_counter_updated) {
    if (error_code) {
      *error_code = ErrorCode::LOOP_COUNTER_NOT_UPDATED;
    }
    return false;
  }
  if (loop_counter_decrement < 1 ||
      loop_counter_decrement >
          1000) {  // prevent exhaustive memory usage; magic number
    if (error_code) {
      *error_code = ErrorCode::LOOP_COUNTER_DECREMENT_INVALID;
    }
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

  if (is_debug) {
    Log::get().debug(
        "[IE] Loop counter decrement: " +
        std::to_string(loop_counter_decrement) +
        ", lower bound: " + std::to_string(loop_counter_lower_bound) +
        ", type: " + Operation::Metadata::get(loop_counter_type).name);
    Log::get().debug(
        "[IE] Num stateful cells: " + std::to_string(stateful_cells.size()) +
        ", num loop counter dependent cells: " +
        std::to_string(loop_counter_dependent_cells.size()) +
        ", is commutative: " + std::to_string(is_commutative));
  }

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
  if (error_code) {
    *error_code = ErrorCode::NON_COMMUTATIVE_OPERATIONS;
  }
  return false;
}

void IncrementalEvaluator::computeStatefulCells() {
  std::set<int64_t> read;
  std::set<int64_t> write;
  stateful_cells.clear();
  for (const auto& op : simple_loop.body.ops) {
    const auto& meta = Operation::Metadata::get(op.type);
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
    for (const auto& op : simple_loop.body.ops) {
      const auto& meta = Operation::Metadata::get(op.type);
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

bool IncrementalEvaluator::checkPostLoop(ErrorCode* error_code) {
  // initialize output cells. all memory cells that are read
  // by the post-loop fragment are output cells.
  std::set<int64_t> write;
  for (const auto& op : simple_loop.post_loop.ops) {
    const auto& meta = Operation::Metadata::get(op.type);
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
  // Post-loop check always succeeds (no error cases currently)
  (void)error_code;  // suppress unused parameter warning
  return true;
}

// ====== Runtime of incremental evaluation ========

void IncrementalEvaluator::initRuntimeData() {
  loop_states.resize(loop_counter_decrement);
  previous_loop_counts.resize(loop_counter_decrement, 0);
  total_loop_steps.resize(loop_counter_decrement, 0);
  argument = offset;
  previous_slice = 0;
}

std::pair<Number, size_t> IncrementalEvaluator::next(bool skip_final_iter,
                                                     bool skip_post_loop) {
  // sanity check: must be initialized
  if (!initialized) {
    throw std::runtime_error("incremental evaluator not initialized");
  }
  if (is_debug) {
    Log::get().debug("[IE] Computing value for n=" + std::to_string(argument));
  }

  // execute pre-loop code
  tmp_state.clear();
  tmp_state.set(Program::INPUT_CELL, argument);
  size_t steps = interpreter.run(pre_loop_filtered, tmp_state);

  // derive loop count and slice
  const int64_t loop_counter_before =
      tmp_state.get(simple_loop.counter).asInt();
  const int64_t new_loop_count =
      std::max<int64_t>(loop_counter_before - loop_counter_lower_bound, 0);
  const int64_t slice = new_loop_count % loop_counter_decrement;

  // calculate number of additional loops
  int64_t additional_loops =
      (new_loop_count - previous_loop_counts[slice]) / loop_counter_decrement;

  // one more iteration may be needed when using trn or max
  if (previous_loop_counts[slice] == 0 &&
      new_loop_count % loop_counter_decrement &&
      (loop_counter_type == Operation::Type::TRN || loop_counter_lower_bound)) {
    additional_loops++;
  }

  if (is_debug) {
    Log::get().debug("[IE] New loop count: " + std::to_string(new_loop_count) +
                     ", additional loops: " + std::to_string(additional_loops) +
                     ", slice: " + std::to_string(slice));
  }

  // init or update loop state
  if (previous_loop_counts[slice] == 0) {
    loop_states[slice] = tmp_state;
  } else {
    for (auto& cell : input_dependent_cells) {
      loop_states[slice].set(cell, tmp_state.get(cell));
    }
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
  auto final_counter_value = Number(slice);
  if (loop_counter_type == Operation::Type::TRN || loop_counter_lower_bound) {
    final_counter_value = Number(loop_counter_lower_bound);
  }
  final_counter_value =
      Semantics::min(final_counter_value, Number(loop_counter_before));

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
