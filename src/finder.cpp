#include "finder.hpp"

#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>

#include "config.hpp"
#include "evaluator_log.hpp"
#include "file.hpp"
#include "log.hpp"
#include "number.hpp"
#include "oeis_list.hpp"
#include "oeis_program.hpp"
#include "oeis_sequence.hpp"
#include "program_util.hpp"
#include "setup.hpp"
#include "util.hpp"

Finder::Finder(const Settings &settings, Evaluator &evaluator)
    : settings(settings),
      evaluator(evaluator),
      optimizer(settings),
      minimizer(settings),
      num_find_attempts(0),
      scheduler(1800)  // 30 minutes
{
  auto config = ConfigLoader::load(settings);
  if (config.matchers.empty()) {
    Log::get().error("No matchers defined", true);
  }

  // create matchers
  matchers.clear();
  for (auto m : config.matchers) {
    try {
      auto matcher = Matcher::Factory::create(m);
      matchers.emplace_back(std::move(matcher));
    } catch (const std::exception &) {
      Log::get().warn("Ignoring error while loading " + m.type + " matcher");
    }
  }
}

void Finder::insert(const Sequence &norm_seq, size_t id) {
  for (auto &matcher : matchers) {
    matcher->insert(norm_seq, id);
  }
}

void Finder::remove(const Sequence &norm_seq, size_t id) {
  for (auto &matcher : matchers) {
    matcher->remove(norm_seq, id);
  }
}

Matcher::seq_programs_t Finder::findSequence(
    const Program &p, Sequence &norm_seq,
    const std::vector<OeisSequence> &sequences) {
  // update memory usage info
  if (num_find_attempts++ % 1000 == 0) {
    bool has_memory = Setup::hasMemory();
    for (auto &matcher : matchers) {
      matcher->has_memory = has_memory;
    }
  }

  // determine largest memory cell to check
  int64_t max_index = 20;  // magic number
  int64_t largest_used_cell;
  tmp_used_cells.clear();
  if (ProgramUtil::getUsedMemoryCells(p, tmp_used_cells, largest_used_cell,
                                      settings.max_memory) &&
      largest_used_cell <= 100) {  // magic number
    max_index = largest_used_cell;
  }

  // interpret program
  tmp_seqs.resize(std::max<size_t>(2, max_index + 1));
  Matcher::seq_programs_t result;
  try {
    evaluator.eval(p, tmp_seqs);
    norm_seq = tmp_seqs[1];
  } catch (const std::exception &) {
    // evaluation error
    return result;
  }
  Program p2 = p;
  p2.push_back(Operation::Type::MOV, Operand::Type::DIRECT,
               Program::OUTPUT_CELL, Operand::Type::DIRECT, 0);
  for (size_t i = 0; i < tmp_seqs.size(); i++) {
    if (i == Program::OUTPUT_CELL) {
      findAll(p, tmp_seqs[i], sequences, result);
    } else {
      p2.ops.back().source.value = i;
      findAll(p2, tmp_seqs[i], sequences, result);
    }
  }
  return result;
}

void Finder::findAll(const Program &p, const Sequence &norm_seq,
                     const std::vector<OeisSequence> &sequences,
                     Matcher::seq_programs_t &result) {
  // collect possible matches
  std::pair<size_t, Program> last(0, Program());
  for (size_t i = 0; i < matchers.size(); i++) {
    tmp_result.clear();
    matchers[i]->match(p, norm_seq, tmp_result);

    // validate the found matches
    for (auto t : tmp_result) {
      auto &s = sequences.at(t.first);
      if (t == last) {
        // Log::get().warn("Ignoring duplicate match for " + s.id_str());
        continue;
      }
      last = t;

      auto expected_seq = s.getTerms(s.existingNumTerms());
      auto res = evaluator.check(t.second, expected_seq,
                                 OeisSequence::DEFAULT_SEQ_LENGTH, t.first);
      if (res.first == status_t::ERROR) {
        notifyInvalidMatch(t.first);
        // Log::get().warn( "Ignoring invalid match for " + s.id_str() );
      } else {
        result.push_back(t);
        // Log::get().info( "Found potential match for " + s.id_str() );
      }
    }
  }
}

void Finder::notifyMinimizerProblem(const Program &p, const std::string &id) {
  Log::get().warn("Program for " + id +
                  " generates wrong result after minimization");
  const std::string f = Setup::getLodaHome() + "debug/minimizer/" + id + ".asm";
  ensureDir(f);
  std::ofstream out(f);
  ProgramUtil::print(p, out);
}

std::pair<std::string, Program> Finder::checkProgramExtended(
    Program program, Program existing, bool is_new, const OeisSequence &seq,
    size_t num_terms, size_t num_usages) {
  std::pair<std::string, Program> result;

  // get the extended sequence
  auto extended_seq = seq.getTerms(num_terms);

  // check the program w/o minimization
  auto check_vanilla = evaluator.check(
      program, extended_seq, OeisSequence::DEFAULT_SEQ_LENGTH, seq.id);
  if (check_vanilla.first == status_t::ERROR) {
    notifyInvalidMatch(seq.id);
    return result;  // not correct
  }

  // the program is correct => update result
  result.second = program;

  // now minimize for default number of terms
  minimizer.optimizeAndMinimize(program, OeisSequence::DEFAULT_SEQ_LENGTH);
  if (program != result.second) {
    // minimization changed program => check the minimized program
    auto check_minimized = evaluator.check(
        program, extended_seq, OeisSequence::DEFAULT_SEQ_LENGTH, seq.id);
    if (check_minimized.first == status_t::ERROR) {
      if (check_vanilla.first == status_t::OK) {
        // looks like the minimization changed the semantics of the program
        notifyMinimizerProblem(result.second, seq.id_str());
      }
      // we ignore the case where the base program has a warning and minimized
      // program an error, because it indicates a problem in the base program
      result.second.ops.clear();
      return result;  // program not ok
    }
  }

  // update result with minimized program
  result.second = program;

  if (is_new) {
    // no additional checks needed for new programs
    result.first = "Found";
  } else {
    // now we are in the "update" case
    // compare (minimized) program with existing programs
    result.first = isOptimizedBetter(existing, result.second, seq.id, num_terms,
                                     num_usages);
  }

  // clear result program if it's no good
  if (result.first.empty()) {
    result.second.ops.clear();
  }
  return result;
}

std::pair<std::string, Program> Finder::checkProgramBasic(
    Program program, Program existing, bool is_new, const OeisSequence &seq,
    const std::string &change_type, size_t previous_hash,
    size_t num_default_terms, size_t num_usages) {
  static const std::string first = "Found";
  std::pair<std::string, Program> result;  // empty string indicates no update

  // additional metadata checks for program update
  if (!is_new) {
    // check if another miner already submitted a program for this sequence
    if (change_type == first) {
      Log::get().debug("Skipping update of " + seq.id_str() +
                       " because program is not new");
      return result;
    }
    // fall back to default validation if metadata is missing
    if (change_type.empty() || !previous_hash) {
      Log::get().debug(
          "Falling back to default validation due to missing metadata");
      return checkProgramExtended(program, existing, is_new, seq,
                                  num_default_terms, num_usages);
    }
    // compare with hash of existing program
    if (previous_hash != OeisProgram::getTransitiveProgramHash(existing)) {
      Log::get().debug("Skipping update of " + seq.id_str() +
                       " because of hash mismatch");
      return result;
    }
  }

  // get the default-length sequence
  auto default_seq = seq.getTerms(OeisSequence::DEFAULT_SEQ_LENGTH);

  // check the program
  auto check = evaluator.check(program, default_seq,
                               OeisSequence::DEFAULT_SEQ_LENGTH, seq.id);
  if (check.first == status_t::ERROR) {
    notifyInvalidMatch(seq.id);
    return result;  // not correct
  }

  // the program is correct => update result
  result.first = is_new ? first : change_type;
  result.second = program;
  return result;
}

size_t getBadOpsCount(const Program &p) {
  // we prefer programs the following programs:
  // - w/o loops that have non-constant args
  // - w/o gcd with powers of a small constant
  size_t num_ops = 0;
  for (auto &op : p.ops) {
    if (op.type == Operation::Type::LPB &&
        op.source.type != Operand::Type::CONSTANT) {
      num_ops++;
    }
    if (op.type == Operation::Type::GCD &&
        op.source.type == Operand::Type::CONSTANT &&
        (Minimizer::getPowerOf(op.source.value) != 0 ||
         Number(100000) < op.source.value)) {
      num_ops++;
    }
  }
  return num_ops;
}

bool isBetterIndirectMemory(const Program &existing, const Program &optimized) {
  return (ProgramUtil::hasIndirectOperand(existing) &&
          !ProgramUtil::hasIndirectOperand(optimized) &&
          !ProgramUtil::hasOp(optimized, Operation::Type::SEQ));
}

bool isBetterIncEval(const Program &existing, const Program &optimized,
                     Evaluator &evaluator) {
  // optimized program supports IE, but existing doesn't
  if (evaluator.supportsIncEval(existing)) {
    return false;
  }
  // avoid overwriting programs w/o loops
  if (!ProgramUtil::hasOp(existing, Operation::Type::LPB) &&
      !ProgramUtil::hasOp(existing, Operation::Type::SEQ)) {
    return false;
  }
  // avoid adding more seq operations
  if (ProgramUtil::numOps(optimized, Operation::Type::SEQ) >
      ProgramUtil::numOps(existing, Operation::Type::SEQ)) {
    return false;
  }
  return evaluator.supportsIncEval(optimized);
}

bool isTrivialPostLoop(const Program &post_loop) {
  if (post_loop.ops.size() > 1) {
    return false;
  }
  if (post_loop.ops.size() == 1 &&
      post_loop.ops[0].type != Operation::Type::MOV) {
    return false;
  }
  return true;
}

bool isBetterIncEval2(const Program &existing, const Program &optimized,
                      Evaluator &evaluator) {
  // both are IE programs. optimized version has trivial post-loop, existing not
  auto &inc = evaluator.getIncEvaluator();
  if (!inc.init(existing)) {
    return false;
  }
  auto post_loop_existing = inc.getPostLoop();
  inc.reset();
  if (!inc.init(optimized)) {
    return false;
  }
  auto post_loop_optimized = inc.getPostLoop();
  inc.reset();
  return (isTrivialPostLoop(post_loop_optimized) &&
          !isTrivialPostLoop(post_loop_existing));
}

bool isBetterLogEval(const Program &existing, const Program &optimized) {
  // optimized version has log complexity, existing does not
  return (ProgramUtil::hasOp(existing, Operation::Type::LPB) &&
          !LogarithmicEvaluator::hasLogarithmicComplexity(existing) &&
          LogarithmicEvaluator::hasLogarithmicComplexity(optimized));
}

std::string Finder::isOptimizedBetter(Program existing, Program optimized,
                                      const OeisSequence &seq, size_t num_terms,
                                      size_t num_usages) {
  static const std::string not_better;

  // ====== STATIC CODE CHECKS ========

  // check if there are illegal recursions
  // why is this not detected by the interpreter?
  for (auto &op : optimized.ops) {
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

  // check if there are loops with contant number of iterations involved
  auto info = ProgramUtil::findConstantLoop(optimized);
  if (info.has_constant_loop) {
    // independently of the existing program, we stop here because
    // otherwise it yields fake optimization of constant loops
    return not_better;
  }

  // compare number of "bad" operations
  auto optimized_bad_count = getBadOpsCount(optimized);
  auto existing_bad_count = getBadOpsCount(existing);
  if (optimized_bad_count < existing_bad_count) {
    return "Simpler";
  } else if (optimized_bad_count > existing_bad_count) {
    return not_better;  // worse
  }

  // check indirect memory
  if (isBetterIndirectMemory(existing, optimized)) {
    return "Simpler";
  } else if (isBetterIndirectMemory(optimized, existing)) {
    return not_better;  // worse
  }

  // check if the optimized program has logarithmic complexity
  if (isBetterLogEval(existing, optimized)) {
    return "Faster (log)";
  } else if (isBetterLogEval(optimized, existing)) {
    return not_better;  // worse
  }

  // consider incremental evaluation only if the program is not used
  // a lot by other programs. if it is used a lot, we prefer faster programs
  if (num_usages < 10) {  // magic number
    // check if the optimized program supports incremental evaluation
    if (isBetterIncEval(existing, optimized, evaluator)) {
      return "Faster (IE)";
    } else if (isBetterIncEval(optimized, existing, evaluator)) {
      return not_better;  // worse
    }
    // check if programs support incremental evaluation and optimized is simpler
    if (isBetterIncEval2(existing, optimized, evaluator)) {
      return "Simpler";
    } else if (isBetterIncEval2(optimized, existing, evaluator)) {
      return not_better;  // worse
    }
  }

  // ======= EVALUATION CHECKS =========

  // get extended sequence
  auto terms = seq.getTerms(num_terms);
  if (terms.empty()) {
    Log::get().error("Error fetching b-file for " + seq.id_str(), true);
  }

  // ensure a minimum number of known terms before comparing
  if (terms.size() < OeisSequence::DEFAULT_SEQ_LENGTH) {
    // Log::get().warn(
    //    "Skipping further comparison because there are too few terms");
    return not_better;
  }

  // evaluate optimized program for fixed number of terms
  num_terms = std::min<size_t>(num_terms, terms.size());
  num_terms = std::max<size_t>(num_terms, OeisSequence::EXTENDED_SEQ_LENGTH);
  Sequence tmp;
  evaluator.clearCaches();
  auto optimized_steps = evaluator.eval(optimized, tmp, num_terms, false);
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
  const auto existing_steps = evaluator.eval(existing, tmp, num_terms, false);
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

  //  compare number of execution cycles
  double existing_total = existing_steps.total;
  double optimized_total = optimized_steps.total;
  if (existing_total > (optimized_total * THRESHOLD_FASTER)) {
    return "Faster";
  } else if (optimized_steps.total > existing_steps.total) {  // no threshold
    return not_better;
  }

  return not_better;  // not better or worse => no change
}

void Finder::notifyInvalidMatch(size_t id) {
  if (invalid_matches.find(id) == invalid_matches.end()) {
    invalid_matches[id] = 1;
  } else {
    invalid_matches[id]++;
  }
  if (scheduler.isTargetReached()) {
    scheduler.reset();
    Log::get().debug("Saving " + std::to_string(invalid_matches.size()) +
                     " invalid matches");
    OeisList::mergeMap(OeisList::INVALID_MATCHES_FILE, invalid_matches);
  }
}

void Finder::logSummary(size_t loaded_count) {
  std::stringstream buf;
  buf << "Matcher compaction ratios: ";
  for (size_t i = 0; i < matchers.size(); i++) {
    if (i > 0) buf << ", ";
    buf << matchers[i]->getName() << ": " << std::setprecision(3)
        << matchers[i]->getCompationRatio() << "%";
  }
  Log::get().debug(buf.str());
}
