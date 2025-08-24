#pragma once

#include <chrono>

#include "eval/evaluator_inc.hpp"
#include "eval/evaluator_vir.hpp"
#include "eval/interpreter.hpp"
#include "eval/range_generator.hpp"
#include "math/sequence.hpp"

class steps_t {
 public:
  steps_t();
  size_t min;
  size_t max;
  size_t total;
  size_t runs;
  void add(size_t s);
  void add(const steps_t &s);
};

enum class status_t { OK, WARNING, ERROR };

typedef int64_t eval_mode_t;
constexpr eval_mode_t EVAL_REGULAR = 1;
constexpr eval_mode_t EVAL_INCREMENTAL = 2;
constexpr eval_mode_t EVAL_VIRTUAL = 4;
constexpr eval_mode_t EVAL_ALL = EVAL_REGULAR | EVAL_INCREMENTAL | EVAL_VIRTUAL;

class Evaluator {
 public:
  explicit Evaluator(const Settings &settings, eval_mode_t eval_modes,
                     bool check_range);

  steps_t eval(const Program &p, Sequence &seq, int64_t num_terms = -1,
               const bool throw_on_error = true);

  steps_t eval(const Program &p, std::vector<Sequence> &seqs,
               int64_t num_terms = -1);

  std::pair<status_t, steps_t> check(const Program &p,
                                     const Sequence &expected_seq,
                                     int64_t num_required_terms = -1,
                                     UID id = UID());

  bool supportsEvalModes(const Program &p, eval_mode_t eval_modes);

  IncrementalEvaluator &getIncEvaluator() { return inc_evaluator; }

  void clearCaches();

 private:
  const Settings &settings;
  Interpreter interpreter;
  IncrementalEvaluator inc_evaluator;
  VirtualEvaluator vir_evaluator;
  RangeGenerator range_generator;
  const bool use_inc_eval;
  const bool use_vir_eval;
  const bool check_range;
  const bool check_eval_time;
  const bool is_debug;
  std::chrono::time_point<std::chrono::steady_clock> start_time;

  Range generateRange(const Program &p, int64_t inputUpperBound);

  void checkEvalTime() const;
};
