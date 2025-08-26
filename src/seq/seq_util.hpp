#pragma once

#include "base/uid.hpp"
#include "math/number.hpp"

class SequenceUtil {
 public:
  static const size_t DEFAULT_SEQ_LENGTH;

  static const size_t EXTENDED_SEQ_LENGTH;

  static const size_t FULL_SEQ_LENGTH;

  static bool isTooBig(const Number& n);

  static std::string getOeisUrl(UID id);

  static std::string getSeqsFolder(char domain);
};