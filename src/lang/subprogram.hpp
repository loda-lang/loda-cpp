#pragma once

#include <map>

#include "lang/program.hpp"

struct EmbeddedSequenceProgram {
  int64_t start_pos, end_pos;
  int64_t input_cell, output_cell;
};

class Subprogram {
 public:
  static size_t replaceAllExact(Program &main, const Program &search,
                                const Program &replace);

  static int64_t search(const Program &main, const Program &sub,
                        std::map<int64_t, int64_t> &cell_map);

  static bool canUnfold(Operation::Type type);

  static bool unfold(Program &main, int64_t pos = -1);

  static bool autoUnfold(Program &main);

  static bool shouldFold(const Program &main);

  static bool fold(Program &main, Program sub, size_t subId,
                   std::map<int64_t, int64_t> &cell_map, int64_t maxMemory);

  static std::vector<EmbeddedSequenceProgram> findEmbeddedSequencePrograms(
      const Program &main, int64_t min_length, int64_t min_loops_outside,
      int64_t min_loops_inside);

  static int64_t annotateEmbeddedSequencePrograms(Program &main,
                                                  int64_t min_length,
                                                  int64_t min_loops_outside,
                                                  int64_t min_loops_inside);
};
