#pragma once

#include "lang/program.hpp"

class VirtualSequence {
 public:
  struct Result {
    int64_t start_pos, end_pos;
    int64_t input_cell, output_cell;
  };

  static std::vector<Result> findVirtualSequencePrograms(
      const Program &main, int64_t min_length, int64_t min_loops_outside,
      int64_t min_loops_inside);

  static int64_t annotateVirtualSequencePrograms(Program &main,
                                                 int64_t min_length,
                                                 int64_t min_loops_outside,
                                                 int64_t min_loops_inside);
};
