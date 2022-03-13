#include "finder.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>

#include "config.hpp"
#include "file.hpp"
#include "number.hpp"
#include "oeis_list.hpp"
#include "oeis_sequence.hpp"
#include "program_util.hpp"
#include "setup.hpp"
#include "util.hpp"

Finder::Finder(const Settings &settings, Evaluator &evaluator)
    : settings(settings),
      evaluator(evaluator),
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
      largest_used_cell <= 100) {
    max_index = largest_used_cell;
  }

  // interpret program
  tmp_seqs.resize(std::max<size_t>(2, max_index + 1));
  Matcher::seq_programs_t result;
  try {
    evaluator.eval(p, tmp_seqs);
    norm_seq = tmp_seqs[1];
  } catch (const std::exception &) {
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

std::pair<bool, Program> Finder::checkAndMinimize(const Program &p,
                                                  const OeisSequence &seq) {
  // Log::get().info( "Checking and minimizing program for " + seq.id_str() );

  std::pair<status_t, steps_t> check;
  std::pair<bool, Program> result;
  result.second = p;

  // get the extended sequence
  auto extended_seq = seq.getTerms(OeisSequence::EXTENDED_SEQ_LENGTH);

  // check the program w/o minimization
  check = evaluator.check(p, extended_seq, OeisSequence::DEFAULT_SEQ_LENGTH,
                          seq.id);
  result.first = (check.first != status_t::ERROR);  // we allow warnings
  if (!result.first) {
    notifyInvalidMatch(seq.id);
    return result;  // not correct
  }
  const auto basic_check_result = check.first;

  // minimize for default number of terms
  minimizer.optimizeAndMinimize(
      result.second, OeisSequence::DEFAULT_SEQ_LENGTH);  // default length
  check = evaluator.check(result.second, extended_seq,
                          OeisSequence::DEFAULT_SEQ_LENGTH, seq.id);
  result.first = (check.first != status_t::ERROR);  // we allow warnings
  if (result.first) {
    return result;
  }

  if (basic_check_result == status_t::OK) {
    // if we got here, the minimization changed the semantics of the program
    Log::get().warn("Program for " + seq.id_str() +
                    " generates wrong result after minimization with " +
                    std::to_string(OeisSequence::DEFAULT_SEQ_LENGTH) +
                    " terms");
    std::string f =
        Setup::getLodaHome() + "debug/minimizer/" + seq.id_str() + ".asm";
    ensureDir(f);
    std::ofstream out(f);
    ProgramUtil::print(p, out);
  }

  return result;
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
    OeisList::mergeMap("invalid_matches.txt", invalid_matches);
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
