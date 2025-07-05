#include "mine/finder.hpp"

#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>

#include "lang/analyzer.hpp"
#include "lang/constants.hpp"
#include "lang/program_util.hpp"
#include "lang/subprogram.hpp"
#include "mine/config.hpp"
#include "oeis/invalid_matches.hpp"
#include "oeis/oeis_list.hpp"
#include "oeis/oeis_program.hpp"
#include "oeis/oeis_sequence.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"
#include "sys/util.hpp"

Finder::Finder(const Settings &settings, Evaluator &evaluator)
    : settings(settings),
      evaluator(evaluator),
      optimizer(settings),
      minimizer(settings),
      num_find_attempts(0),
      invalid_matches(),
      checker(settings, evaluator, minimizer, invalid_matches) {
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
    for (const auto &matcher : matchers) {
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
      auto num_required = OeisProgram::getNumRequiredTerms(t.second);
      auto res = evaluator.check(t.second, expected_seq, num_required, t.first);
      if (res.first == status_t::ERROR) {
        invalid_matches.insert(t.first);
        // Log::get().warn( "Ignoring invalid match for " + s.id_str() );
      } else {
        result.push_back(t);
        // Log::get().info( "Found potential match for " + s.id_str() );
      }
    }
  }
}

void Finder::notifyUnfoldOrMinimizeProblem(const Program &p,
                                           const std::string &id) {
  Log::get().warn("Program for " + id +
                  " generates wrong result after unfold/minimize");
  const std::string f = Setup::getLodaHome() + "debug/minimizer/" + id + ".asm";
  ensureDir(f);
  std::ofstream out(f);
  ProgramUtil::print(p, out);
}

bool hasBadConstant(const Program &p) {
  auto constants = Constants::getAllConstants(p, true);
  return std::any_of(constants.begin(), constants.end(), [](const Number &c) {
    return (Minimizer::getPowerOf(c) != 0 ||
            Number(100000) < c);  // magic number
  });
}

bool hasBadLoop(const Program &p) {
  return std::any_of(p.ops.begin(), p.ops.end(), [](const Operation &op) {
    return (op.type == Operation::Type::LPB &&
            (op.source.type != Operand::Type::CONSTANT ||
             op.source.value != Number::ONE));
  });
}

bool isSimpler(const Program &existing, const Program &optimized) {
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

bool isBetterIncEval(const Program &existing, const Program &optimized,
                     Evaluator &evaluator) {
  // avoid overwriting programs w/o loops
  if (!ProgramUtil::hasOp(existing, Operation::Type::LPB) &&
      !ProgramUtil::hasOp(existing, Operation::Type::SEQ)) {
    return false;
  }
  bool optimized_has_seq = ProgramUtil::hasOp(optimized, Operation::Type::SEQ);
  return (!evaluator.supportsIncEval(existing) &&
          evaluator.supportsIncEval(optimized) && !optimized_has_seq);
}
