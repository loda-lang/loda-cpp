#include "mine/checker.hpp"

#include <algorithm>
#include <sstream>

#include "eval/minimizer.hpp"
#include "lang/constants.hpp"
#include "lang/program_util.hpp"
#include "lang/subprogram.hpp"
#include "oeis/oeis_program.hpp"
#include "oeis/oeis_sequence.hpp"
#include "sys/log.hpp"

namespace {

bool hasBadConstant(const Program& p) {
  auto constants = Constants::getAllConstants(p, true);
  return std::any_of(constants.begin(), constants.end(), [](const Number& c) {
    return (Minimizer::getPowerOf(c) != 0 || Number(100000) < c);
  });
}

bool hasBadLoop(const Program& p) {
  return std::any_of(p.ops.begin(), p.ops.end(), [](const Operation& op) {
    return (op.type == Operation::Type::LPB &&
            (op.source.type != Operand::Type::CONSTANT ||
             op.source.value != Number::ONE));
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
  if (ProgramUtil::hasIndirectOperand(existing) &&
      !ProgramUtil::hasIndirectOperand(optimized) && !optimized_has_seq) {
    return true;
  }
  return false;
}

bool isBetterIncEval(const Program& existing, const Program& optimized,
                     Evaluator& evaluator) {
  if (!ProgramUtil::hasOp(existing, Operation::Type::LPB) &&
      !ProgramUtil::hasOp(existing, Operation::Type::SEQ)) {
    return false;
  }
  bool optimized_has_seq = ProgramUtil::hasOp(optimized, Operation::Type::SEQ);
  return (!evaluator.supportsIncEval(existing) &&
          evaluator.supportsIncEval(optimized) && !optimized_has_seq);
}

}  // namespace

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
  auto num_check = OeisProgram::getNumCheckTerms(full_check);
  auto num_required = OeisProgram::getNumRequiredTerms(program);
  auto num_minimize = OeisProgram::getNumMinimizationTerms(program);
  auto extended_seq = seq.getTerms(num_check);
  auto check_vanilla =
      evaluator.check(program, extended_seq, num_required, seq.id);
  if (check_vanilla.first == status_t::ERROR) {
    invalid_matches.insert(seq.id);
    return result;
  }
  result.program = program;
  Subprogram::autoUnfold(program);
  minimizer.optimizeAndMinimize(program, num_minimize);
  if (program != result.program) {
    num_required = OeisProgram::getNumRequiredTerms(program);
    auto check_minimized =
        evaluator.check(program, extended_seq, num_required, seq.id);
    if (check_minimized.first == status_t::ERROR) {
      if (check_vanilla.first == status_t::OK) {
        Log::get().warn("Program for " + ProgramUtil::idStr(seq.id) +
                        " generates wrong result after unfold/minimize");
      }
      result.program.ops.clear();
      return result;
    }
  }
  result.program = program;
  if (is_new) {
    result.status = "Found";
  } else {
    result.status = isOptimizedBetter(existing, result.program, seq, full_check,
                                      num_usages);
  }
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
  check_result_t result;
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
  auto num_required = OeisProgram::getNumRequiredTerms(program);
  auto terms = seq.getTerms(num_required);
  auto check = evaluator.check(program, terms, num_required, seq.id);
  if (check.first == status_t::ERROR) {
    invalid_matches.insert(seq.id);
    return result;
  }
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
  if (optimized.ops.empty()) {
    return not_better;
  }
  if (optimized == existing) {
    return not_better;
  }
  if (isSimpler(existing, optimized)) {
    return "Simpler";
  } else if (isSimpler(optimized, existing)) {
    return not_better;
  }
  if (!full_check && num_usages < 5) {
    if (isBetterIncEval(existing, optimized, evaluator)) {
      return "Faster (IE)";
    } else if (isBetterIncEval(optimized, existing, evaluator)) {
      return not_better;
    }
  }
  // ======= EVALUATION CHECKS =========
  auto num_check = OeisProgram::getNumCheckTerms(full_check);
  auto terms = seq.getTerms(num_check);
  if (terms.empty()) {
    Log::get().error("Error fetching b-file for " + ProgramUtil::idStr(seq.id),
                     true);
  }
  num_check = std::min<size_t>(num_check, terms.size());
  num_check = std::max<size_t>(num_check, OeisSequence::EXTENDED_SEQ_LENGTH);
  Sequence tmp;
  evaluator.clearCaches();
  auto optimized_steps = evaluator.eval(optimized, tmp, num_check, false);
  if (Signals::HALT) {
    return not_better;
  }
  const int64_t s = terms.size();
  if (tmp.get_first_delta_lt(Number::ZERO) >= s ||
      tmp.get_first_delta_lt(Number::ONE) >= s) {
    return not_better;
  }
  evaluator.clearCaches();
  const auto existing_steps = evaluator.eval(existing, tmp, num_check, false);
  if (Signals::HALT) {
    return not_better;
  }
  double existing_terms = existing_steps.runs;
  double optimized_terms = optimized_steps.runs;
  if (optimized_terms > (existing_terms * THRESHOLD_BETTER)) {
    return "Better";
  } else if (existing_steps.runs > optimized_steps.runs) {
    return not_better;
  }
  double existing_total = existing_steps.total;
  double optimized_total = optimized_steps.total;
  if (existing_total > (optimized_total * THRESHOLD_FASTER)) {
    return "Faster";
  } else if (optimized_steps.total > existing_steps.total) {
    return not_better;
  }
  return not_better;
}

std::string Checker::compare(Program p1, Program p2, const std::string& name1,
                             const std::string& name2, const OeisSequence& seq,
                             size_t num_terms, size_t num_usages) {
  auto result = isOptimizedBetter(p1, p2, seq, num_terms, num_usages);
  if (!result.empty()) {
    lowerString(result);
    return name2 + " program is " + result;
  }
  result = isOptimizedBetter(p2, p1, seq, num_terms, num_usages);
  if (!result.empty()) {
    lowerString(result);
    return name1 + " program is " + result;
  }
  return "Both programs are equivalent";
}
