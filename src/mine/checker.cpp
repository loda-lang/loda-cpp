#include "mine/checker.hpp"

#include <algorithm>
#include <limits>
#include <sstream>

#include "eval/minimizer.hpp"
#include "lang/constants.hpp"
#include "lang/program_cache.hpp"
#include "lang/program_util.hpp"
#include "lang/subprogram.hpp"
#include "oeis/oeis_program.hpp"
#include "oeis/oeis_sequence.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"

bool hasBadConstant(const Program& p) {
  auto constants = Constants::getAllConstants(p, true);
  return std::any_of(constants.begin(), constants.end(), [](const Number& c) {
    return (Minimizer::getPowerOf(c) != 0 ||
            Number(100000) < c);  // magic number
  });
}

bool hasBadLoop(const Program& p) {
  return std::any_of(p.ops.begin(), p.ops.end(), [](const Operation& op) {
    return (op.type == Operation::Type::LPB &&
            (op.source.type != Operand::Type::CONSTANT ||
             op.source.value != Number::ONE));
  });
}

bool hasIndirectOperand(const Program& p) {
  if (ProgramUtil::hasIndirectOperand(p)) {
    return true;
  }
  // check if the program uses a sequence operation with an indirect operand
  ProgramCache cache;
  constexpr int64_t dummy_id = std::numeric_limits<int>::max();
  cache.insert(dummy_id, p);
  auto collected = cache.collect(dummy_id);
  return std::any_of(collected.begin(), collected.end(), [](const auto& it) {
    return ProgramUtil::hasIndirectOperand(it.second);
  });
}

bool isSimpler(const Program& existing, const Program& optimized) {
  bool optimized_has_seq = ProgramUtil::hasOp(optimized, Operation::Type::SEQ);
  if (hasBadConstant(existing) && !hasBadConstant(optimized) &&
      !optimized_has_seq) {
    return true;
  }
  if (hasBadLoop(existing) && !hasBadLoop(optimized) && !optimized_has_seq) {
    return true;
  }
  auto info_existing = Constants::findConstantLoop(existing);
  auto info_optimized = Constants::findConstantLoop(optimized);
  if (info_existing.has_constant_loop && !info_optimized.has_constant_loop &&
      !optimized_has_seq) {
    return true;
  }
  if (hasIndirectOperand(existing) && !hasIndirectOperand(optimized)) {
    return true;
  }
  return false;
}

bool isBetterIncEval(const Program& existing, const Program& optimized,
                     Evaluator& evaluator) {
  // avoid overwriting programs w/o loops
  if (!ProgramUtil::hasOp(existing, Operation::Type::LPB) &&
      !ProgramUtil::hasOp(existing, Operation::Type::SEQ)) {
    return false;
  }
  bool optimized_has_seq = ProgramUtil::hasOp(optimized, Operation::Type::SEQ);
  return (!evaluator.supportsEvalModes(existing, EVAL_INCREMENTAL) &&
          evaluator.supportsEvalModes(optimized, EVAL_INCREMENTAL) &&
          !optimized_has_seq);
}

Checker::Checker(const Settings& settings, Evaluator& evaluator,
                 Minimizer& minimizer, InvalidMatches& invalid_matches)
    : evaluator(evaluator),
      minimizer(minimizer),
      invalid_matches(invalid_matches),
      optimizer(settings) {}

check_result_t Checker::checkProgramExtended(Program program, Program existing,
                                             bool is_new,
                                             const OeisSequence& seq,
                                             bool full_check,
                                             size_t num_usages) {
  check_result_t result;

  // get the extended sequence and number of required terms
  auto num_check = OeisProgram::getNumCheckTerms(full_check);
  auto num_required = OeisProgram::getNumRequiredTerms(program);
  auto num_minimize = OeisProgram::getNumMinimizationTerms(program);
  auto extended_seq = seq.getTerms(num_check);

  // check the program w/o minimization
  auto check_vanilla =
      evaluator.check(program, extended_seq, num_required, seq.id);
  if (check_vanilla.first == status_t::ERROR) {
    invalid_matches.insert(seq.id);
    return result;  // not correct
  }

  // the program is correct => update result
  result.program = program;

  // auto-unfold seq operations
  Subprogram::autoUnfold(program);

  // minimize based on number of terminating terms
  minimizer.optimizeAndMinimize(program, num_minimize);
  if (program != result.program) {
    // minimization changed program => check the minimized program
    num_required = OeisProgram::getNumRequiredTerms(program);
    auto check_minimized =
        evaluator.check(program, extended_seq, num_required, seq.id);
    if (check_minimized.first == status_t::ERROR) {
      if (check_vanilla.first == status_t::OK) {
        // looks like the minimization changed the semantics of the program
        notifyUnfoldOrMinimizeProblem(result.program,
                                      ProgramUtil::idStr(seq.id));
      }
      // we ignore the case where the base program has a warning and minimized
      // program an error, because it indicates a problem in the base program
      result.program.ops.clear();
      return result;
    }
  }

  // update result with minimized program
  result.program = program;
  if (is_new) {
    // no additional checks needed for new programs
    result.status = "Found";
  } else {
    // now we are in the "update" case
    // compare (minimized) program with existing programs
    result.status = isOptimizedBetter(existing, result.program, seq, full_check,
                                      num_usages);
  }

  // clear result program if it's no good
  if (result.status.empty()) {
    result.program.ops.clear();
  }
  return result;
}

check_result_t Checker::checkProgramBasic(const Program& program,
                                          const Program& existing, bool is_new,
                                          const OeisSequence& seq,
                                          const std::string& change_type,
                                          size_t previous_hash, bool full_check,
                                          size_t num_usages) {
  static const std::string first = "Found";
  check_result_t result;  // empty string indicates no update

  // additional metadata checks for program update
  if (!is_new) {
    // check if another miner already submitted a program for this sequence
    if (change_type == first) {
      Log::get().debug("Skipping update of " + ProgramUtil::idStr(seq.id) +
                       " because program is not new");
      return result;
    }
    // fall back to default validation if metadata is missing
    if (change_type.empty() || !previous_hash) {
      Log::get().debug(
          "Falling back to default validation due to missing metadata");
      return checkProgramExtended(program, existing, is_new, seq, full_check,
                                  num_usages);
    }
    // compare with hash of existing program
    if (previous_hash != OeisProgram::getTransitiveProgramHash(existing)) {
      Log::get().debug("Skipping update of " + ProgramUtil::idStr(seq.id) +
                       " because of hash mismatch");
      return result;
    }
  }

  // get the number of required terms and the sequence
  auto num_required = OeisProgram::getNumRequiredTerms(program);
  auto terms = seq.getTerms(num_required);

  // check the program
  auto check = evaluator.check(program, terms, num_required, seq.id);
  if (check.first == status_t::ERROR) {
    invalid_matches.insert(seq.id);  // not correct
    return result;
  }

  // the program is correct => update result
  result.status = is_new ? first : change_type;
  result.program = program;
  return result;
}

std::string Checker::isOptimizedBetter(Program existing, Program optimized,
                                       const OeisSequence& seq, bool full_check,
                                       size_t num_usages) {
  static constexpr double THRESHOLD_BETTER = 1.05;
  static constexpr double THRESHOLD_FASTER = 1.1;
  static const std::string not_better;

  // ====== STATIC CODE CHECKS ========

  // check if there are illegal recursions
  // why is this not detected by the interpreter?
  for (const auto& op : optimized.ops) {
    if (op.type == Operation::Type::SEQ &&
        (op.source.type != Operand::Type::CONSTANT ||
         op.source.value == Number(seq.id))) {
      return not_better;
    }
  }

  // remove nops...
  optimizer.removeNops(existing);
  optimizer.removeNops(optimized);

  // we want at least one operation (avoid empty program for A000004)
  if (optimized.ops.empty()) {
    return not_better;
  }

  // if the programs are the same, no need to evaluate them
  if (optimized == existing) {
    return not_better;
  }

  if (isSimpler(existing, optimized)) {
    return "Simpler";
  } else if (isSimpler(optimized, existing)) {
    return not_better;
  }

  // consider incremental evaluation only for programs that are not
  // used by other programs and that don't require a full check
  if (!full_check && num_usages < 5) {  // magic number
    // check if the optimized program supports incremental evaluation
    if (isBetterIncEval(existing, optimized, evaluator)) {
      return "Faster (IE)";
    } else if (isBetterIncEval(optimized, existing, evaluator)) {
      return not_better;
    }
  }

  // ======= EVALUATION CHECKS =========

  // get extended sequence
  auto num_check = OeisProgram::getNumCheckTerms(full_check);
  auto terms = seq.getTerms(num_check);
  if (terms.empty()) {
    Log::get().error("Error fetching b-file for " + ProgramUtil::idStr(seq.id),
                     true);
  }

  // evaluate optimized program for fixed number of terms
  num_check = std::min<size_t>(num_check, terms.size());
  num_check = std::max<size_t>(num_check, OeisSequence::EXTENDED_SEQ_LENGTH);
  Sequence tmp;
  evaluator.clearCaches();
  auto optimized_steps = evaluator.eval(optimized, tmp, num_check, false);
  if (Signals::HALT) {
    return not_better;  // interrupted evaluation
  }

  // check if the first decreasing/non-increasing term is beyond the known
  // sequence terms => fake "better" program
  const int64_t s = terms.size();
  if (tmp.get_first_delta_lt(Number::ZERO) >= s ||  // decreasing
      tmp.get_first_delta_lt(Number::ONE) >= s) {   // non-increasing
    return not_better;                              // => fake "better" program
  }

  // evaluate existing program for same number of terms
  evaluator.clearCaches();
  const auto existing_steps = evaluator.eval(existing, tmp, num_check, false);
  if (Signals::HALT) {
    return not_better;  // interrupted evaluation
  }

  // check number of successfully computed terms
  // we don't try to optimize for number of terms
  double existing_terms = existing_steps.runs;
  double optimized_terms = optimized_steps.runs;
  if (optimized_terms > (existing_terms * THRESHOLD_BETTER)) {
    return "Better";
  } else if (existing_steps.runs > optimized_steps.runs) {  // no threshold
    return not_better;
  }

  //  compare number of execution steps
  double existing_total = existing_steps.total;
  double optimized_total = optimized_steps.total;
  if (existing_total > (optimized_total * THRESHOLD_FASTER)) {
    return "Faster";
  } else if (optimized_steps.total > existing_steps.total) {  // no threshold
    return not_better;
  }

  return not_better;  // not better or worse => no change
}

std::string Checker::compare(Program p1, Program p2, const std::string& name1,
                             const std::string& name2, const OeisSequence& seq,
                             bool full_check, size_t num_usages) {
  auto result = isOptimizedBetter(p1, p2, seq, full_check, num_usages);
  if (!result.empty()) {
    lowerString(result);
    return name2 + " program is " + result;
  }
  result = isOptimizedBetter(p2, p1, seq, full_check, num_usages);
  if (!result.empty()) {
    lowerString(result);
    return name1 + " program is " + result;
  }
  return "Both programs are equivalent";
}

void Checker::notifyUnfoldOrMinimizeProblem(const Program& p,
                                            const std::string& id) {
  Log::get().warn("Program for " + id +
                  " generates wrong result after unfold/minimize");
  const std::string f = Setup::getLodaHome() + "debug/minimizer/" + id + ".asm";
  ensureDir(f);
  std::ofstream out(f);
  ProgramUtil::print(p, out);
}
