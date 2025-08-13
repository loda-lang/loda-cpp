#include "mine/matcher.hpp"

#include "eval/optimizer.hpp"
#include "eval/semantics.hpp"
#include "mine/reducer.hpp"
#include "sys/log.hpp"

// --- Factory --------------------------------------------------------

Matcher::UPtr Matcher::Factory::create(const Matcher::Config &config) {
  Matcher::UPtr result;
  if (config.type == "direct") {
    result.reset(new DirectMatcher(config.backoff));
  } else if (config.type == "linear1") {
    result.reset(new LinearMatcher(config.backoff));
  } else if (config.type == "linear2") {
    result.reset(new LinearMatcher2(config.backoff));
  } else if (config.type == "delta") {
    result.reset(new DeltaMatcher(config.backoff));
  } else if (config.type == "binary") {
    result.reset(new DigitMatcher("binary", 2, config.backoff));
  } else if (config.type == "decimal") {
    result.reset(new DigitMatcher("decimal", 10, config.backoff));
  } else {
    Log::get().error("Unknown matcher type: " + config.type, true);
  }
  return result;
}

// --- AbstractMatcher --------------------------------------------------------

template <class T>
void AbstractMatcher<T>::insert(const Sequence &norm_seq, size_t id) {
  auto reduced = reduce(norm_seq, false);
  if (!reduced.first.empty()) {
    data[id] = reduced.second;
    ids[reduced.first].push_back(UID('A', id));
  }
}

template <class T>
void AbstractMatcher<T>::remove(const Sequence &norm_seq, size_t id) {
  auto reduced = reduce(norm_seq, false);
  if (!reduced.first.empty()) {
    ids.remove(reduced.first, UID('A', id));
    data.erase(id);
  }
}

template <class T>
void AbstractMatcher<T>::match(const Program &p, const Sequence &norm_seq,
                               seq_programs_t &result) const {
  if (!shouldMatchSequence(norm_seq)) {
    return;
  }
  auto reduced = reduce(norm_seq, true);
  if (!shouldMatchSequence(reduced.first) && norm_seq != reduced.first) {
    return;
  }
  auto it = ids.find(reduced.first);
  if (it != ids.end()) {
    for (auto id : it->second) {
      Program copy = p;
      if (extend(copy, data.at(id.number()), reduced.second)) {
        result.push_back(std::pair<size_t, Program>(id.number(), copy));
        if (backoff && (Random::get().gen() % 10) == 0)  // magic number
        {
          // avoid to many matches for the same sequence
          break;
        }
      }
    }
  }
}

template <class T>
bool AbstractMatcher<T>::shouldMatchSequence(const Sequence &seq) const {
  if (backoff) {
    if (match_attempts.find(seq) != match_attempts.end()) {
      // Log::get().debug( "Back off matching of already matched sequence " +
      // seq.to_string() );
      return false;
    }
    if ((has_memory || match_attempts.size() < 1000) &&  // magic number
        (Random::get().gen() % 10) == 0)                 // magic number
    {
      match_attempts.insert(seq);
    }
  }
  return true;
}

// --- Direct Matcher ---------------------------------------------------------

std::pair<Sequence, int> DirectMatcher::reduce(const Sequence &seq,
                                               bool match) const {
  std::pair<Sequence, int> result;
  result.first = seq;
  result.second = 0;
  return result;
}

bool DirectMatcher::extend(Program &p, int base, int gen) const { return true; }

// --- Linear Matcher ---------------------------------------------------------

std::pair<Sequence, line_t> LinearMatcher::reduce(const Sequence &seq,
                                                  bool match) const {
  std::pair<Sequence, line_t> result;
  result.first = seq;
  result.second.offset = Reducer::truncate(result.first);
  result.second.factor = Reducer::shrink(result.first);
  return result;
}

bool LinearMatcher::extend(Program &p, line_t base, line_t gen) const {
  return Extender::linear1(p, gen, base);
}

std::pair<Sequence, line_t> LinearMatcher2::reduce(const Sequence &seq,
                                                   bool match) const {
  std::pair<Sequence, line_t> result;
  result.first = seq;
  result.second.factor = Reducer::shrink(result.first);
  result.second.offset = Reducer::truncate(result.first);
  return result;
}

bool LinearMatcher2::extend(Program &p, line_t base, line_t gen) const {
  return Extender::linear2(p, gen, base);
}

// --- Delta Matcher ----------------------------------------------------------

const int64_t DeltaMatcher::MAX_DELTA = 4;  // magic number

std::pair<Sequence, delta_t> DeltaMatcher::reduce(const Sequence &seq,
                                                  bool match) const {
  std::pair<Sequence, delta_t> result;
  result.first = seq;
  result.second = Reducer::delta(result.first, MAX_DELTA);
  return result;
}

bool DeltaMatcher::extend(Program &p, delta_t base, delta_t gen) const {
  if (base.offset == gen.offset && base.factor == gen.factor) {
    return Extender::delta_it(p, base.delta - gen.delta);
  } else {
    if (!Extender::delta_it(p, -gen.delta)) {
      return false;
    }
    line_t base_line;
    base_line.offset = base.offset;
    base_line.factor = base.factor;
    line_t gen_line;
    gen_line.offset = gen.offset;
    gen_line.factor = gen.factor;
    if (!Extender::linear1(p, gen_line, base_line)) {
      return false;
    }
    if (!Extender::delta_it(p, base.delta)) {
      return false;
    }
  }
  return true;
}

// --- Digit Matcher ----------------------------------------------------------

std::pair<Sequence, int64_t> DigitMatcher::reduce(const Sequence &seq,
                                                  bool match) const {
  std::pair<Sequence, int64_t> result;
  result.first = seq;
  result.second = Reducer::digit(result.first, num_digits);
  if (!match) {
    for (auto &n : seq) {
      if (n < Number::ZERO || !(n < num_digits_big)) {
        result.first.clear();
        break;
      }
    }
  }
  return result;
}

bool DigitMatcher::extend(Program &p, int64_t base, int64_t gen) const {
  return Extender::digit(p, num_digits, base - gen);
}
