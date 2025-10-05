#pragma once

#include <set>

#include "eval/interpreter.hpp"
#include "lang/analyzer.hpp"

// Incremental Evaluator (IE) for simple loop programs. This class is designed
// for a subset of LODA programs that contain simple loops, and is not a general
// evaluator for all LODA programs. IE offers much faster evaluation by
// computing each new sequence term incrementally: the loop body is executed
// only a fixed number of times per term (typically just once), rather than
// recomputing from scratch.
//
// The evaluator achieves this by tracking and updating the state from the
// previous iteration. Whether IE can be used for a given program is determined
// by static code analysis.
//
// To check if your program is compatible with IE, call the init() function. If
// it returns true, you can efficiently compute successive terms by repeatedly
// calling next().
//
class IncrementalEvaluator {
 public:
  // Detailed error codes for initialization failures
  enum class ErrorCode {
    OK = 0,
    // Simple loop extraction error (1-99)
    NOT_A_SIMPLE_LOOP = 1,
    // Pre-loop check errors (100-199)
    LOOP_COUNTER_NOT_INPUT_DEPENDENT = 100,
    PRELOOP_UNSUPPORTED_OPERATION = 101,
    PRELOOP_NON_CONSTANT_OPERAND = 102,
    // Loop body check errors (200-299)
    LOOP_COUNTER_NOT_UPDATED = 200,
    LOOP_COUNTER_DECREMENT_INVALID = 201,
    LOOP_COUNTER_UPDATE_INVALID = 202,
    INPUT_DEPENDENT_CELL_READ = 203,
    INPUT_DEPENDENT_SOURCE_USED = 204,
    NON_COMMUTATIVE_OPERATIONS = 205,
  };
 public:
  explicit IncrementalEvaluator(Interpreter& interpreter);

  void reset();

  // Initialize the IE using a program. IE can be applied only if this function
  // returns true. If error_code is provided, it will be set to the detailed
  // error code when initialization fails.
  bool init(const Program& program, bool skip_input_transform = false,
            bool skip_offset = false, ErrorCode* error_code = nullptr);

  // Compute the next term and step count.
  std::pair<Number, size_t> next(bool skip_final_iter = false,
                                 bool skip_post_loop = false);

  inline const SimpleLoopProgram& getSimpleLoop() const { return simple_loop; }
  inline const Program& getPreLoopFiltered() const { return pre_loop_filtered; }
  inline int64_t getLoopCounterDecrement() const {
    return loop_counter_decrement;
  }
  inline int64_t getLoopCounterLowerBound() { return loop_counter_lower_bound; }
  inline const std::set<int64_t>& getInputDependentCells() const {
    return input_dependent_cells;
  }
  inline const std::set<int64_t>& getLoopCounterDependentCells() const {
    return loop_counter_dependent_cells;
  }
  inline const std::set<int64_t>& getStatefulCells() const {
    return stateful_cells;
  }
  inline const std::set<int64_t>& getOutputCells() const {
    return output_cells;
  }
  inline const std::vector<Memory>& getLoopStates() const {
    return loop_states;
  }
  inline int64_t getPreviousSlice() const { return previous_slice; }
  bool isInputDependent(const Operand& op) const;

  // Get the last error code from initialization
  inline ErrorCode getLastErrorCode() const { return last_error_code; }

 private:
  bool checkPreLoop(bool skip_input_transform, ErrorCode* error_code);
  bool checkLoopBody(ErrorCode* error_code);
  bool checkPostLoop(ErrorCode* error_code);
  void computeStatefulCells();
  void computeLoopCounterDependentCells();
  void initRuntimeData();

  Interpreter& interpreter;

  // program fragments and metadata
  SimpleLoopProgram simple_loop;
  Program pre_loop_filtered;
  std::set<int64_t> output_cells;
  std::set<int64_t> stateful_cells;
  std::set<int64_t> input_dependent_cells;
  std::set<int64_t> loop_counter_dependent_cells;
  int64_t loop_counter_decrement;
  int64_t loop_counter_lower_bound;
  int64_t offset;
  Operation::Type loop_counter_type;
  bool initialized;
  const bool is_debug;
  ErrorCode last_error_code;

  // runtime data
  int64_t argument;
  Memory tmp_state;
  std::vector<Memory> loop_states;
  std::vector<int64_t> previous_loop_counts;
  std::vector<size_t> total_loop_steps;
  int64_t previous_slice;
};
