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

void interruptEval() {
  throw std::runtime_error("interrupted evaluation due to halt signal");
}

steps_t Evaluator::eval(const Program &p, Sequence &seq, int64_t num_terms,
                        const bool throw_on_error, const bool use_inc_eval) {
  if (num_terms < 0) {
    num_terms = settings.num_terms;
  }
  seq.resize(num_terms);
  Memory mem;
  steps_t steps;
  size_t s;
  const bool use_inc = use_inc_eval && inc_evaluator.init(p);
  std::pair<Number, size_t> inc_result;
  for (int64_t i = 0; i < num_terms; i++) {
    try {
      if (use_inc) {
        inc_result = inc_evaluator.next();
        seq[i] = inc_result.first;
        s = inc_result.second;
      } else {
        mem.clear();
        mem.set(Program::INPUT_CELL, i);
        s = interpreter.run(p, mem);
        seq[i] = mem.get(Program::OUTPUT_CELL);
      }
      if (Signals::HALT) {
        interruptEval();
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
  // note: we can't use the incremental evaluator here
  for (int64_t i = 0; i < num_terms; i++) {
    mem.clear();
    mem.set(Program::INPUT_CELL, i);
    steps.add(interpreter.run(p, mem));
    for (size_t s = 0; s < seqs.size(); s++) {
      seqs[s][i] = mem.get(s);
    }
    if (Signals::HALT) {
      interruptEval();
    }
  }
  return steps;
}

std::pair<status_t, steps_t> Evaluator::check(const Program &p,
                                              const Sequence &expected_seq,
                                              int64_t num_terminating_terms,
                                              int64_t id,
                                              const bool use_inc_eval) {
  if (num_terminating_terms < 0) {
    num_terminating_terms = expected_seq.size();
  }
  std::pair<status_t, steps_t> result;
  Memory mem;
  // clear cache to correctly detect recursion errors
  interpreter.clearCaches();
  const bool use_inc = use_inc_eval && inc_evaluator.init(p);
  std::pair<Number, size_t> inc_result;
  Number out;
  for (size_t i = 0; i < expected_seq.size(); i++) {
    try {
      if (use_inc) {
        inc_result = inc_evaluator.next();
        out = inc_result.first;
      } else {
        mem.clear();
        mem.set(Program::INPUT_CELL, i);
        result.second.add(interpreter.run(p, mem, id));
        out = mem.get(Program::OUTPUT_CELL);
      }
      if (Signals::HALT) {
        interruptEval();
      }
    } catch (const std::exception &e) {
      if (settings.print_as_b_file) {
        std::cout << std::string(e.what()) << std::endl;
      }
      result.first = ((int64_t)i >= num_terminating_terms) ? status_t::WARNING
                                                           : status_t::ERROR;
      return result;
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

bool Evaluator::supportsIncEval(const Program &p) {
  bool result = inc_evaluator.init(p);
  inc_evaluator.reset();
  return result;
}

void Evaluator::clearCaches() { interpreter.clearCaches(); }
