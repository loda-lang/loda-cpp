#include "evaluator.hpp"

#include <sstream>

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

Evaluator::Evaluator(const Settings &settings)
    : settings(settings),
      interpreter(settings),
      inc_evaluator(interpreter),
      is_debug(Log::get().level == Log::Level::DEBUG) {}

steps_t Evaluator::eval(const Program &p, Sequence &seq, int64_t num_terms,
                        bool throw_on_error) {
  if (num_terms < 0) {
    num_terms = settings.num_terms;
  }
  seq.resize(num_terms);
  Memory mem;
  steps_t steps;
  size_t s;
  const bool use_inc = inc_evaluator.init(p);
  for (int64_t i = 0; i < num_terms; i++) {
    if (use_inc) {
      seq[i] = inc_evaluator.next();
      // TODO: update steps
    } else {
      mem.clear();
      mem.set(Program::INPUT_CELL, i);
      try {
        s = interpreter.run(p, mem);
      } catch (const std::exception &) {
        seq.resize(i);
        if (throw_on_error) {
          throw;
        } else {
          return steps;
        }
      }
      steps.add(s);
      seq[i] = settings.use_steps ? s : mem.get(Program::OUTPUT_CELL);
    }
    if (settings.print_as_b_file) {
      std::cout << (settings.print_as_b_file_offset + i) << " " << seq[i]
                << std::endl;
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
  Memory mem;
  steps_t steps;
  // TODO: try to use incremental evaluator
  // TODO: update steps
  for (int64_t i = 0; i < num_terms; i++) {
    mem.clear();
    mem.set(Program::INPUT_CELL, i);
    steps.add(interpreter.run(p, mem));
    for (size_t s = 0; s < seqs.size(); s++) {
      seqs[s][i] = mem.get(s);
    }
  }
  return steps;
}

std::pair<status_t, steps_t> Evaluator::check(const Program &p,
                                              const Sequence &expected_seq,
                                              int64_t num_terminating_terms,
                                              int64_t id) {
  if (num_terminating_terms < 0) {
    num_terminating_terms = expected_seq.size();
  }
  std::pair<status_t, steps_t> result;
  Memory mem;
  // clear cache to correctly detect recursion errors
  interpreter.clearCaches();
  const bool use_inc = inc_evaluator.init(p);
  Number out;
  for (size_t i = 0; i < expected_seq.size(); i++) {
    if (use_inc) {
      out = inc_evaluator.next();
    } else {
      mem.clear();
      mem.set(Program::INPUT_CELL, i);
      try {
        result.second.add(interpreter.run(p, mem, id));
      } catch (const std::exception &e) {
        if (settings.print_as_b_file) {
          std::cout << std::string(e.what()) << std::endl;
        }
        result.first = ((int64_t)i >= num_terminating_terms) ? status_t::WARNING
                                                             : status_t::ERROR;
        return result;
      }
      out = mem.get(Program::OUTPUT_CELL);
    }
    if (out != expected_seq[i]) {
      if (settings.print_as_b_file) {
        std::cout << (settings.print_as_b_file_offset + i) << " " << out
                  << " -> expected " << expected_seq[i] << std::endl;
      }
      result.first = status_t::ERROR;
      return result;
    }
    if (settings.print_as_b_file) {
      std::cout << (settings.print_as_b_file_offset + i) << " "
                << expected_seq[i] << std::endl;
    }
  }
  result.first = status_t::OK;
  return result;
}
