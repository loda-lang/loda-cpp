#pragma once

#include <set>

#include "lang/program.hpp"

class Constants {
 public:
  class LoopInfo {
   public:
    bool has_constant_loop;
    size_t index_lpb;
    Number constant_value;
  };

  static std::set<Number> getAllConstants(const Program &p,
                                          bool arithmetic_only);

  static Number getLargestConstant(const Program &p);

  static LoopInfo findConstantLoop(const Program &p);
};
