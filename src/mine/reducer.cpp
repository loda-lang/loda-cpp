#include "mine/reducer.hpp"

#include "lang/semantics.hpp"
#include "sys/util.hpp"

Number Reducer::truncate(Sequence &seq) {
  if (seq.empty()) {
    return Number::ZERO;
  }
  // get minimum positive value; no negative values are allowed
  Number min = Number::INF;
  for (auto &v : seq) {
    if (v < Number::ZERO) {
      return Number::ZERO;
    } else if (min == Number::INF || v < min) {
      min = v;
    }
  }
  if (Number::ZERO < min && min != Number::INF) {
    for (size_t i = 0; i < seq.size(); i++) {
      seq[i] = Semantics::sub(seq[i], min);
    }
  }
  return min;
}

Number Reducer::shrink(Sequence &seq) {
  Number factor = Number::INF;
  for (size_t i = 0; i < seq.size(); i++) {
    if (seq[i] != Number::ZERO) {
      if (factor == Number::INF) {
        factor = Semantics::abs(seq[i]);
      } else if (factor != Number::ONE) {
        factor = Semantics::gcd(factor, Semantics::abs(seq[i]));
      }
    }
  }
  if (factor == Number::INF || factor == Number::ZERO) {
    factor = Number::ONE;
  }
  if (factor != Number::ONE) {
    for (size_t i = 0; i < seq.size(); i++) {
      seq[i] = Semantics::div(seq[i], factor);
    }
  }
  return factor;
}

delta_t Reducer::delta(Sequence &seq, int64_t max_delta) {
  delta_t result;
  result.delta = 0;
  result.offset = Number::ZERO;
  result.factor = Number::ONE;
  const size_t size = seq.size();
  Sequence next;
  next.resize(size);
  for (int64_t i = 0; i < max_delta; i++) {
    bool ok = true;
    bool same = true;
    for (size_t j = 0; j < size; j++) {
      Number p = (j == 0) ? Number::ZERO : seq[j - 1];
      if (!(seq[j] < p)) {
        next[j] = Semantics::sub(seq[j], p);
        if (p != 0) {
          same = false;
        }
      } else {
        ok = false;
        break;
      }
    }
    if (ok && !same) {
      seq = next;
      result.delta++;
    } else {
      break;
    }
  }
  result.offset = truncate(seq);
  result.factor = shrink(seq);
  //  Log::get().info(
  //      "Reduced sequence to " + seq.to_string() + " using delta=" +
  //      std::to_string( result.delta ) + ", offset="
  //          + std::to_string( result.offset ) + ", factor=" + std::to_string(
  //          result.factor ) );
  return result;
}

int64_t Reducer::digit(Sequence &seq, int64_t num_digits) {
  std::vector<size_t> count;
  count.resize(num_digits, 0);
  for (auto &n : seq) {
    count[((Semantics::mod(n, num_digits)).asInt() + num_digits) %
          num_digits]++;
  }
  Number index;
  size_t max = 0;
  for (int64_t i = 0; i < num_digits; i++) {
    if (count[i] > max) {
      index = Number(i);
      max = count[i];
    }
  }
  const Number d(num_digits);
  for (int64_t i = 0; i < static_cast<int64_t>(seq.size()); i++) {
    seq[i] = Semantics::mod(
        Semantics::add(Semantics::mod(Semantics::sub(seq[i], index), d), d), d);
  }
  //  Log::get().info(
  //      "Reduced sequence to " + seq.to_string() + " using num_digits=" +
  //      std::to_string( num_digits ) + ", offset="
  //          + std::to_string( index ) );
  return index.asInt();
}
