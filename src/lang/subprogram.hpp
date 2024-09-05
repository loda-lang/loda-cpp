#pragma once

#include <map>

#include "lang/program.hpp"

class Subprogram {
 public:
  static size_t replaceAllExact(Program &main, const Program &search,
                                const Program &replace);

  static int64_t search(const Program &main, const Program &sub,
                        std::map<int64_t, int64_t> &cell_map);

  static bool unfold(Program &main);

  static bool autoUnfold(Program &main);

  static bool shouldFold(const Program &main);

  static bool fold(Program &main, Program sub, size_t subId,
                   std::map<int64_t, int64_t> &cell_map);
};
