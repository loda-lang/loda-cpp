#include "eval/evaluator.hpp"

#include <sstream>

#include "lang/program_util.hpp"
#include "sys/log.hpp"

steps_t::steps_t() : min(0), max(0), total(0), runs(0) {}

void steps_t::add(size_t s) {
  min = std::min(min, s);
  max = std::max(max, s);
  total += s;
  runs++;
}

void steps_t::add(const steps_t &s) {
  min = std::min(min, s.min);
  max = std::max(max, s.max);
  total += s.total;
  runs += s.runs;
}

Evaluator::Evaluator(const Settings &settings, const bool use_inc_eval,
                     bool check_range)
    : settings(settings),
      interpreter(settings),
      inc_evaluator(interpreter),
      use_inc_eval(use_inc_eval),
      check_range(check_range),
      check_eval_time(settings.max_eval_secs >= 0),
      is_debug(Log::get().level == Log::Level::DEBUG) {}

steps_t Evaluator::eval(const Program &p, Sequence &seq, int64_t num_terms,
                        const bool throw_on_error) {
  if (num_terms < 0) {
    num_terms = settings.num_terms;
  }
  seq.resize(num_terms);
  if (check_eval_time) {
    start_time = std::chrono::steady_clock::now();
  }
  Memory mem;
  steps_t steps;
  size_t s;
  const bool use_inc = use_inc_eval && inc_evaluator.init(p);
  std::pair<Number, size_t> inc_result;
  const int64_t offset = ProgramUtil::getOffset(p);
  for (int64_t i = 0; i < num_terms; i++) {
    try {
      if (use_inc) {
        inc_result = inc_evaluator.next();
        seq[i] = inc_result.first;
        s = inc_result.second;
      } else {
        mem.clear();
        mem.set(Program::INPUT_CELL, i + offset);
        s = interpreter.run(p, mem);
        seq[i] = mem.get(Program::OUTPUT_CELL);
      }
      if (check_eval_time) {
        checkEvalTime();
      }
    } catch (const std::exception &) {
      seq.resize(i);
      if (throw_on_error) {
        throw;
      } else {
        return steps;
      }
    }

    steps.add(s);
    if (settings.use_steps) {
      seq[i] = s;
    }
    if (settings.print_as_b_file) {
      std::cout << (offset + i) << " " << seq[i] << std::endl;
    }
  }
  if (is_debug) {
    std::stringstream buf;
    buf << "Evaluated program to sequence " << seq;
    Log::get().debug(buf.str());
  }
  return steps;
}

steps_t Evaluator::eval(const Program &p, std::vector<Sequence> &seqs,
                        int64_t num_terms) {
  if (num_terms < 0) {
    num_terms = settings.num_terms;
  }
  for (size_t s = 0; s < seqs.size(); s++) {
    seqs[s].resize(num_terms);
  }
  if (check_eval_time) {
    start_time = std::chrono::steady_clock::now();
  }
  Memory mem;
  steps_t steps;
  // note: we can't use the incremental evaluator here
  const int64_t offset = ProgramUtil::getOffset(p);
  for (int64_t i = 0; i < num_terms; i++) {
    mem.clear();
    mem.set(Program::INPUT_CELL, i + offset);
    steps.add(interpreter.run(p, mem));
    for (size_t s = 0; s < seqs.size(); s++) {
      seqs[s][i] = mem.get(s);
    }
    if (check_eval_time) {
      checkEvalTime();
    }
  }
  return steps;
}

std::string rangeStr(const Range &r, int64_t n) {
  return r.toString("a(" + std::to_string(n) + ")");
}

Range Evaluator::generateRange(const Program &p, int64_t inputUpperBound) {
  RangeMap ranges;
  if (!range_generator.generate(p, ranges, Number(inputUpperBound))) {
    ranges.clear();
  }
  return ranges.get(Program::OUTPUT_CELL);
}

void printb(int64_t index, const std::string &val) {
  std::cout << index << " " << val << std::endl;
}

std::pair<status_t, steps_t> Evaluator::check(const Program &p,
                                              const Sequence &expected_seq,
                                              int64_t num_required_terms,
                                              int64_t id) {
  if (num_required_terms < 0) {
    num_required_terms = expected_seq.size();
  }
  if (check_eval_time) {
    start_time = std::chrono::steady_clock::now();
  }
  // compute range
  Range range;
  const int64_t offset = ProgramUtil::getOffset(p);
  if (check_range) {
    range = generateRange(p, offset + expected_seq.size() - 1);
  }
  // clear cache to correctly detect recursion errors
  interpreter.clearCaches();
  const bool use_inc = use_inc_eval && inc_evaluator.init(p);
  std::pair<Number, size_t> inc_result;
  std::pair<status_t, steps_t> result;
  result.first = status_t::OK;
  Memory mem;
  Number out;
  for (size_t i = 0; i < expected_seq.size(); i++) {
    const int64_t index = i + offset;
    if (result.first == status_t::OK) {
      try {
        if (use_inc) {
          inc_result = inc_evaluator.next();
          out = inc_result.first;
        } else {
          mem.clear();
          mem.set(Program::INPUT_CELL, i + offset);
          result.second.add(interpreter.run(p, mem, id));
          out = mem.get(Program::OUTPUT_CELL);
        }
        if (check_eval_time) {
          checkEvalTime();
        }
      } catch (const std::exception &e) {
        if (static_cast<int64_t>(i) < num_required_terms) {
          result.first = status_t::ERROR;
          if (settings.print_as_b_file) {
            printb(index, "-> " + std::string(e.what()));
          }
          return result;
        } else {
          result.first = status_t::WARNING;
          if (!check_range || range.isUnbounded()) {
            return result;
          }
        }
      }
      if (out != expected_seq[i]) {
        if (settings.print_as_b_file) {
          printb(index, out.to_string() + " -> expected " +
                            expected_seq[i].to_string());
        }
        result.first = status_t::ERROR;
        return result;
      }
    }
    if (check_range && !range.check(expected_seq[i])) {
      if (settings.print_as_b_file) {
        printb(index,
               out.to_string() + " -> expected " + expected_seq[i].to_string());
      }
      result.first = status_t::ERROR;
      return result;
    }
    if (settings.print_as_b_file) {
      std::string val_str = (result.first == status_t::OK)
                                ? expected_seq[i].to_string()
                                : rangeStr(range, offset + i);
      printb(index, val_str);
    }
  }
  return result;
}

bool Evaluator::supportsIncEval(const Program &p) {
  bool result = inc_evaluator.init(p);
  inc_evaluator.reset();
  return result;
}

void Evaluator::clearCaches() { interpreter.clearCaches(); }

void Evaluator::checkEvalTime() const {
  const int64_t millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start_time)
                             .count();
  if (millis > 1000 * settings.max_eval_secs) {
    throw std::runtime_error("maximum evaluation time exceeded");
  }
}
