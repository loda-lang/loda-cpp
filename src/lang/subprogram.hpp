#pragma once

#include <map>

#include "lang/program.hpp"

class Subprogram {
 public:
  static int64_t search(const Program &main, const Program &sub,
                        std::map<int64_t, int64_t> &cell_map);

  static size_t replaceAllExact(Program &main, const Program &search,
                                const Program &replace);
};
