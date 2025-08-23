#include "seq/sequence_util.hpp"

#include "math/big_number.hpp"

const size_t SequenceUtil::DEFAULT_SEQ_LENGTH = 80;  // magic number

const size_t SequenceUtil::EXTENDED_SEQ_LENGTH = 1000;  // magic number

const size_t SequenceUtil::FULL_SEQ_LENGTH = 100000;  // magic number

bool SequenceUtil::isTooBig(const Number& n) {
  if (n == Number::INF) {
    return true;
  }
  if (USE_BIG_NUMBER) {
    return n.getNumUsedWords() >
           static_cast<int64_t>(BigNumber::NUM_WORDS / 4);  // magic number
  } else {
    static const int64_t NUM_INF = std::numeric_limits<int64_t>::max();
    return (n.value > (NUM_INF / 1000)) || (n.value < (NUM_INF / -1000));
  }
}

std::string SequenceUtil::getOeisUrl(UID id) {
  return "https://oeis.org/" + id.string();
}
