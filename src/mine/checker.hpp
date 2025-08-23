#pragma once

#include <string>

#include "eval/evaluator.hpp"
#include "eval/minimizer.hpp"
#include "eval/optimizer.hpp"
#include "mine/matcher.hpp"
#include "oeis/invalid_matches.hpp"

class ManagedSequence;

struct check_result_t {
  std::string status;
  Program program;
};

class Checker {
 public:
  Checker(const Settings& settings, Evaluator& evaluator, Minimizer& minimizer,
          InvalidMatches& invalid_matches);

  check_result_t checkProgramExtended(Program program, Program existing,
                                      bool is_new, const ManagedSequence& seq,
                                      bool full_check, size_t num_usages);

  check_result_t checkProgramBasic(const Program& program,
                                   const Program& existing, bool is_new,
                                   const ManagedSequence& seq,
                                   const std::string& change_type,
                                   size_t previous_hash, bool full_check,
                                   size_t num_usages);

  std::string isOptimizedBetter(Program existing, Program optimized,
                                const ManagedSequence& seq, bool full_check,
                                size_t num_usages);

  std::string compare(Program p1, Program p2, const std::string& name1,
                      const std::string& name2, const ManagedSequence& seq,
                      bool full_check, size_t num_usages);

 private:
  Evaluator& evaluator;
  Minimizer& minimizer;
  InvalidMatches& invalid_matches;
  Optimizer optimizer;

  void notifyUnfoldOrMinimizeProblem(const Program& p, const std::string& id);
};
